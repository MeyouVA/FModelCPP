// End-to-end tests for the IO Store layer: FIoStoreTocResource (toc parsing), IoStoreReader (directory
// index -> Files, chunk resolution, block-based extraction) and the provider integration (.utoc
// registration, ById, cooked-index payload lookup).
//
// Every test authors a real .utoc + .ucas byte-for-byte and mounts them through the actual reader, the
// same approach as test_pak.cpp. Compression goes through the fake tiling codec in the Oodle slot;
// encryption through the CustomEncryption delegate (byte-wise XOR) — AES itself is pinned in test_aes.cpp.
//
// The perfect-hash test computes its seed table with the same FNV-1a the reader uses. That is deliberate:
// what is under test there is the ROUTING (seed lookup, negative-seed direct index, overflow fallback),
// not the hash function, and a real perfect-hash table cannot be authored without running the hash.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "Compression/Compression.h"
#include "Encryption/Aes/FAesKey.h"
#include "FileProvider/Vfs/AbstractVfsFileProvider.h"
#include "UE4/Exceptions/ParserException.h"
#include "UE4/IO/IoStoreReader.h"
#include "UE4/IO/Objects/FIoStatus.h"
#include "UE4/IO/Objects/FIoStoreEntry.h"
#include "UE4/Objects/Core/Misc/FGuid.h"
#include "UE4/Readers/FByteArchive.h"

using namespace CUE4Parse::UE4::IO;
using namespace CUE4Parse::UE4::IO::Objects;
using CUE4Parse::Compression::CompressionAlgorithm;
using CUE4Parse::Compression::CompressionMethod;
using CUE4Parse::Encryption::Aes::FAesKey;
using CUE4Parse::FileProvider::Vfs::AbstractVfsFileProvider;
using CUE4Parse::UE4::Objects::Core::Misc::FGuid;
using CUE4Parse::UE4::Readers::FArchive;
using CUE4Parse::UE4::Readers::FByteArchive;
using CUE4Parse::UE4::Versions::VersionContainer;
using CUE4Parse::UE4::VirtualFileSystem::IAesVfsReader;
using CUE4Parse::Utils::StringComparer;

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL: %s (line %d)\n", #cond, __LINE__); ++g_failures; } } while (0)

// --------------------------------------------------------------------------------------------------
struct Writer
{
    std::vector<uint8_t> Bytes;

    int64_t Pos() const { return static_cast<int64_t>(Bytes.size()); }

    template <typename T>
    void Put(T value)
    {
        const auto* p = reinterpret_cast<const uint8_t*>(&value);
        Bytes.insert(Bytes.end(), p, p + sizeof(T));
    }

    void Raw(const std::string& data) { Bytes.insert(Bytes.end(), data.begin(), data.end()); }
    void Raw(const std::vector<uint8_t>& data) { Bytes.insert(Bytes.end(), data.begin(), data.end()); }
    void Zeros(int64_t count) { Bytes.insert(Bytes.end(), static_cast<size_t>(count), 0); }

    void FString(const std::string& s)
    {
        Put<int32_t>(static_cast<int32_t>(s.size() + 1));
        Raw(s);
        Put<uint8_t>(0);
    }

    // The 5-byte big-endian halves of FIoOffsetAndLength.
    void Put5BE(uint64_t v)
    {
        Put<uint8_t>(static_cast<uint8_t>(v >> 32));
        Put<uint8_t>(static_cast<uint8_t>(v >> 24));
        Put<uint8_t>(static_cast<uint8_t>(v >> 16));
        Put<uint8_t>(static_cast<uint8_t>(v >> 8));
        Put<uint8_t>(static_cast<uint8_t>(v));
    }
};

static std::shared_ptr<FArchive> Archive(std::string name, std::vector<uint8_t> bytes)
{
    return std::make_shared<FByteArchive>(std::move(name), std::move(bytes));
}

static std::string ToString(const std::vector<uint8_t>& bytes)
{
    return std::string(bytes.begin(), bytes.end());
}

static void RegisterFakeCodec()
{
    static bool registered = false;
    if (registered) return;
    registered = true;
    CUE4Parse::Compression::Compression::RegisterDecompressor(
        CompressionAlgorithm::Oodle,
        [](const uint8_t* src, int srcLen, uint8_t* dst, int dstLen, int& written) {
            if (srcLen <= 0) return false;
            for (int i = 0; i < dstLen; ++i) dst[i] = src[i % srcLen];
            written = dstLen;
            return true;
        });
}

// --------------------------------------------------------------------------------------------------
// Fixture: three chunks under "../../../Game/", block size 16.
//   fileA.uasset  (chunk 0x1111/ExportBundleData): virtual [0,19)  = blocks 0..1, stored
//   sub/fileB.bin (chunk 0x2222/BulkData)        : virtual [32,42) = block 2, "Oodle" tile "WXYZ" -> 10
//   fileA.ubulk   (chunk 0x1111/BulkData)        : virtual [48,53) = block 3, stored
// The ucas stores every block in a 16-byte raw slot, so it is 64 bytes.
// --------------------------------------------------------------------------------------------------
static const std::string kFileAText = "Zen package bytes!!"; // 19
static const std::string kFileBTile = "WXYZ";
static const std::string kFileBText = "WXYZWXYZWX"; // tiled to 10
static const std::string kBulkText = "bulk!"; // 5

static const FIoChunkId kChunkA(0x1111, 0, EIoChunkType::ExportBundleData);
static const FIoChunkId kChunkB(0x2222, 0, EIoChunkType::BulkData);
static const FIoChunkId kChunkBulk(0x1111, 0, EIoChunkType::BulkData);

struct TocOptions
{
    uint8_t Version = 2;         // DirectoryIndex
    uint32_t PartitionCount = 0; // pre-PartitionSize tocs leave these zero
    uint64_t PartitionSize = 0;
    bool Encrypted = false;
    uint8_t XorMask = 0x5A;
    // Perfect hash (version >= 5): filled by the writer when non-empty.
    std::vector<int32_t> PerfectHashSeeds;
    std::vector<int32_t> IndicesWithoutPerfectHash;
};

static std::vector<uint8_t> MakeUcas(const TocOptions& opt)
{
    Writer ucas;
    ucas.Raw(kFileAText.substr(0, 16)); // block 0
    ucas.Raw(kFileAText.substr(16));    // block 1 (3 bytes)
    ucas.Zeros(13);
    ucas.Raw(kFileBTile);               // block 2 (4 compressed bytes)
    ucas.Zeros(12);
    ucas.Raw(kBulkText);                // block 3 (5 bytes)
    ucas.Zeros(11);

    if (opt.Encrypted)
        for (uint8_t& b : ucas.Bytes) b ^= opt.XorMask; // every block's raw window, back to back

    return ucas.Bytes;
}

static std::vector<uint8_t> MakeUtoc(const TocOptions& opt)
{
    // --- directory index ---
    Writer index;
    index.FString("../../../Game/");
    constexpr uint32_t INVALID = UINT32_MAX;
    index.Put<int32_t>(2); // directory entries
    //           Name     FirstChild NextSibling FirstFile
    index.Put<uint32_t>(INVALID); index.Put<uint32_t>(1); index.Put<uint32_t>(INVALID); index.Put<uint32_t>(0); // root
    index.Put<uint32_t>(0);       index.Put<uint32_t>(INVALID); index.Put<uint32_t>(INVALID); index.Put<uint32_t>(1); // "sub"
    index.Put<int32_t>(3); // file entries
    //           Name     NextFile   UserData(toc index)
    index.Put<uint32_t>(1); index.Put<uint32_t>(2); index.Put<uint32_t>(0);       // fileA.uasset -> chunk 0
    index.Put<uint32_t>(3); index.Put<uint32_t>(INVALID); index.Put<uint32_t>(1); // fileB.bin    -> chunk 1
    index.Put<uint32_t>(2); index.Put<uint32_t>(INVALID); index.Put<uint32_t>(2); // fileA.ubulk  -> chunk 2
    index.Put<int32_t>(4); // string table
    index.FString("sub");
    index.FString("fileA.uasset");
    index.FString("fileA.ubulk");
    index.FString("fileB.bin");
    while (index.Pos() % 16 != 0) index.Zeros(1); // keep the encrypted form AES-block-aligned

    if (opt.Encrypted)
        for (uint8_t& b : index.Bytes) b ^= opt.XorMask;

    // --- header ---
    Writer toc;
    toc.Raw(std::vector<uint8_t>(FIoStoreTocHeader::TOC_MAGIC.begin(), FIoStoreTocHeader::TOC_MAGIC.end()));
    toc.Put<uint8_t>(opt.Version);
    toc.Zeros(3);                 // reserved
    toc.Put<uint32_t>(144);       // TocHeaderSize
    toc.Put<uint32_t>(3);         // TocEntryCount
    toc.Put<uint32_t>(4);         // TocCompressedBlockEntryCount
    toc.Put<uint32_t>(12);        // TocCompressedBlockEntrySize
    toc.Put<uint32_t>(1);         // CompressionMethodNameCount
    toc.Put<uint32_t>(32);        // CompressionMethodNameLength
    toc.Put<uint32_t>(16);        // CompressionBlockSize
    toc.Put<uint32_t>(static_cast<uint32_t>(index.Pos())); // DirectoryIndexSize
    toc.Put<uint32_t>(opt.PartitionCount);
    toc.Put<uint64_t>(0xC0FFEE);  // ContainerId
    toc.Zeros(16);                // EncryptionKeyGuid
    toc.Put<uint32_t>(8u | (opt.Encrypted ? 2u : 0u)); // ContainerFlags: Indexed (+Encrypted)
    toc.Put<uint32_t>(static_cast<uint32_t>(opt.PerfectHashSeeds.size()));
    toc.Put<uint64_t>(opt.PartitionSize);
    toc.Put<uint32_t>(static_cast<uint32_t>(opt.IndicesWithoutPerfectHash.size()));
    toc.Zeros(4 + 40);            // reserved
    CHECK(toc.Pos() == 144);

    // --- chunk ids ---
    for (const auto& id : {kChunkA, kChunkB, kChunkBulk}) toc.Put(id);

    // --- chunk offset/lengths (virtual, 5-byte big-endian halves) ---
    toc.Put5BE(0);  toc.Put5BE(kFileAText.size());
    toc.Put5BE(32); toc.Put5BE(kFileBText.size());
    toc.Put5BE(48); toc.Put5BE(kBulkText.size());

    // --- perfect hash tables ---
    for (const int32_t seed : opt.PerfectHashSeeds) toc.Put<int32_t>(seed);
    for (const int32_t idx : opt.IndicesWithoutPerfectHash) toc.Put<int32_t>(idx);

    // --- compression blocks: 40-bit ucas offset + 24-bit compressed size, 24-bit uncompressed + method ---
    const auto block = [&toc](uint64_t offset, uint32_t compressed, uint32_t uncompressed, uint8_t method)
    {
        toc.Put<uint64_t>(offset | (static_cast<uint64_t>(compressed) << 40));
        toc.Put<uint32_t>(uncompressed | (static_cast<uint32_t>(method) << 24));
    };
    block(0, 16, 16, 0);
    block(16, 3, 3, 0);
    block(32, 4, 10, 1); // Oodle tile
    block(48, 5, 5, 0);

    // --- compression method names ---
    toc.Raw(std::string("Oodle"));
    toc.Zeros(32 - 5);

    // --- directory index ---
    toc.Raw(index.Bytes);
    return toc.Bytes;
}

static std::shared_ptr<IoStoreReader> MakeReader(const TocOptions& opt, const std::string& name = "test.utoc")
{
    const auto ucas = MakeUcas(opt);
    const uint64_t partitionSize = opt.PartitionCount > 1 ? opt.PartitionSize : ucas.size();
    return std::make_shared<IoStoreReader>(
        Archive(name, MakeUtoc(opt)),
        [ucas, partitionSize](const std::string& path)
        {
            // Split the ucas into partition files on demand.
            size_t partition = 0;
            if (const auto pos = path.find("_s"); pos != std::string::npos)
                partition = static_cast<size_t>(std::stoi(path.substr(pos + 2)));
            const size_t begin = partition * partitionSize;
            const size_t end = std::min(ucas.size(), begin + partitionSize);
            return Archive(path, std::vector<uint8_t>(ucas.begin() + begin, ucas.begin() + end));
        });
}

class TestProvider : public AbstractVfsFileProvider
{
public:
    TestProvider() : AbstractVfsFileProvider(VersionContainer(), StringComparer::OrdinalIgnoreCase()) {}
    void Initialize() override {}
};

// --------------------------------------------------------------------------------------------------

static void TestTocParsing()
{
    const auto reader = MakeReader({});
    const auto& toc = reader->TocResource;

    CHECK(toc.Header->Version == EIoStoreTocVersion::DirectoryIndex);
    CHECK(toc.Header->TocEntryCount == 3);
    CHECK(toc.Header->TocCompressedBlockEntryCount == 4);
    CHECK(toc.Header->CompressionBlockSize == 16);
    CHECK(toc.Header->ContainerId.Id == 0xC0FFEE);
    // Pre-PartitionSize tocs get the single-partition defaults filled in.
    CHECK(toc.Header->PartitionCount == 1);
    CHECK(toc.Header->PartitionSize == UINT64_MAX);
    CHECK(toc.ChunkIds.size() == 3 && toc.ChunkIds[0] == kChunkA);
    CHECK(toc.ChunkOffsetLengths[1].Offset() == 32 && toc.ChunkOffsetLengths[1].Length() == 10);
    CHECK(toc.CompressionMethods.size() == 2);
    CHECK(toc.CompressionMethods[0] == CompressionMethod::None);
    CHECK(toc.CompressionMethods[1] == CompressionMethod::Oodle);
    CHECK(toc.CompressionBlocks[2].Offset() == 32);
    CHECK(toc.CompressionBlocks[2].CompressedSize() == 4);
    CHECK(toc.CompressionBlocks[2].UncompressedSize() == 10);
    CHECK(toc.CompressionBlocks[2].CompressionMethodIndex() == 1);
    CHECK(toc.DirectoryIndexBufferOffset != -1);
    CHECK(reader->HasDirectoryIndex());
    CHECK(!reader->IsEncrypted());
}

static void TestBadMagic()
{
    auto bytes = MakeUtoc({});
    bytes[0] ^= 0xFF;
    bool threw = false;
    try
    {
        IoStoreReader reader(Archive("bad.utoc", bytes), [](const std::string& p) { return Archive(p, {}); });
    }
    catch (const CUE4Parse::UE4::Exceptions::ParserException&) { threw = true; }
    CHECK(threw);
}

static void TestMissingUcas()
{
    bool threw = false;
    try
    {
        IoStoreReader reader(Archive("orphan.utoc", MakeUtoc({})),
                             [](const std::string&) -> std::shared_ptr<FArchive> { throw std::runtime_error("no ucas"); });
    }
    catch (const FIoStatusException& e) { threw = e.ErrorCode == EIoErrorCode::FileOpenFailed; }
    CHECK(threw);
}

static void TestMountAndExtract()
{
    RegisterFakeCodec();

    const auto reader = MakeReader({});
    reader->Mount(StringComparer::OrdinalIgnoreCase());

    CHECK(reader->MountPoint() == "Game/");
    CHECK(reader->FileCount() == 3);
    CHECK(reader->ReadOrder() == 3);

    const auto& files = reader->Files();
    CHECK(files.count("Game/fileA.uasset") == 1);
    CHECK(files.count("Game/sub/fileB.bin") == 1);
    CHECK(files.count("Game/fileA.ubulk") == 1);
    CHECK(files.count("GAME/FILEA.UASSET") == 1); // comparer applies

    auto* entryA = dynamic_cast<FIoStoreEntry*>(files.at("Game/fileA.uasset").get());
    CHECK(entryA != nullptr);
    CHECK(entryA->Offset == 0 && entryA->Size == 19);
    CHECK(entryA->ChunkId() == kChunkA);
    CHECK(entryA->GetCompressionMethod() == CompressionMethod::None);
    CHECK(!entryA->IsEncrypted());
    CHECK(entryA->IsUePackage());
    CHECK(ToString(entryA->Read()) == kFileAText); // spans two blocks

    auto* entryB = dynamic_cast<FIoStoreEntry*>(files.at("Game/sub/fileB.bin").get());
    CHECK(entryB != nullptr);
    CHECK(entryB->GetCompressionMethod() == CompressionMethod::Oodle);
    CHECK(ToString(entryB->Read()) == kFileBText); // decompressed through the fake codec

    CHECK(ToString(files.at("Game/fileA.ubulk")->Read()) == kBulkText);

    auto sub = entryA->CreateReader();
    CHECK(sub->Length == 19 && sub->Name() == "Game/fileA.uasset");

    // Chunk resolution (no perfect hash -> linear scan).
    CHECK(reader->DoesChunkExist(kChunkA));
    CHECK(ToString(reader->Read(kChunkB)) == kFileBText);
    CHECK(!reader->DoesChunkExist(FIoChunkId(0x9999, 0, EIoChunkType::BulkData)));
    bool threw = false;
    try { reader->Read(FIoChunkId(0x9999, 0, EIoChunkType::BulkData)); }
    catch (const std::out_of_range&) { threw = true; }
    CHECK(threw);

    // The uasset and its ubulk share a chunk id; only the package lands in PackageIdIndex.
    CHECK(reader->PackageIdIndex.size() == 1);
    CHECK(reader->PackageIdIndex.count(FPackageId(0x1111)) == 1);

    // A patch toc gets patch read order.
    const auto patch = MakeReader({}, "test_1_P.utoc");
    patch->Mount(StringComparer::Ordinal());
    CHECK(patch->ReadOrder() == 203);
}

static void TestPartitionedUcas()
{
    RegisterFakeCodec();

    TocOptions opt;
    opt.Version = 3; // PartitionSize: the header's partition fields are honoured
    opt.PartitionCount = 2;
    opt.PartitionSize = 32; // blocks 0..1 in test.ucas, blocks 2..3 in test_s1.ucas

    const auto reader = MakeReader(opt);
    CHECK(reader->TocResource.Header->PartitionCount == 2);
    CHECK(reader->ContainerStreams.size() == 2);

    reader->Mount(StringComparer::OrdinalIgnoreCase());
    CHECK(ToString(reader->Files().at("Game/fileA.uasset")->Read()) == kFileAText);   // partition 0
    CHECK(ToString(reader->Files().at("Game/sub/fileB.bin")->Read()) == kFileBText);  // partition 1
    CHECK(ToString(reader->Files().at("Game/fileA.ubulk")->Read()) == kBulkText);     // partition 1
}

static void TestPerfectHashResolution()
{
    RegisterFakeCodec();

    TocOptions opt;
    opt.Version = 5; // PerfectHashWithOverflow
    opt.PartitionCount = 1;
    opt.PartitionSize = UINT64_MAX;

    // Build a valid seed table with the reader's own FNV (see the file header for why that is OK here):
    // chunk 0 through a positive seed, chunk 1 through a negative direct-index seed, chunk 2 through the
    // overflow fallback.
    constexpr uint32_t seedCount = 8;
    constexpr uint32_t chunkCount = 3;
    const uint32_t h0 = static_cast<uint32_t>(kChunkA.HashWithSeed(0) % seedCount);
    const uint32_t h1 = static_cast<uint32_t>(kChunkB.HashWithSeed(0) % seedCount);
    const uint32_t h2 = static_cast<uint32_t>(kChunkBulk.HashWithSeed(0) % seedCount);
    CHECK(h0 != h1 && h0 != h2 && h1 != h2); // if this ever fails, pick different chunk ids

    int32_t positiveSeed = 0;
    for (int32_t s = 1; s < 100000; ++s)
    {
        if (kChunkA.HashWithSeed(s) % chunkCount == 0) { positiveSeed = s; break; }
    }
    CHECK(positiveSeed > 0);

    opt.PerfectHashSeeds.assign(seedCount, 0);
    opt.PerfectHashSeeds[h0] = positiveSeed;                        // slot = HashWithSeed(seed) % count = 0
    opt.PerfectHashSeeds[h1] = -2;                                  // seedAsIndex = 1 -> direct slot 1
    opt.PerfectHashSeeds[h2] = -(static_cast<int32_t>(chunkCount) + 1); // seedAsIndex = 3 >= count -> fallback
    opt.IndicesWithoutPerfectHash = {2};

    const auto reader = MakeReader(opt);
    CHECK(reader->TocImperfectHashMapFallback.has_value());
    CHECK(reader->TocImperfectHashMapFallback->size() == 1);

    FIoOffsetAndLength ol;
    CHECK(reader->TryResolve(kChunkA, ol) && ol.Offset() == 0 && ol.Length() == 19);
    CHECK(reader->TryResolve(kChunkB, ol) && ol.Offset() == 32 && ol.Length() == 10);
    CHECK(reader->TryResolve(kChunkBulk, ol) && ol.Offset() == 48 && ol.Length() == 5);
    CHECK(!reader->DoesChunkExist(FIoChunkId(0x9999, 0, EIoChunkType::BulkData)));

    // Extraction is unaffected by the hash tables.
    reader->Mount(StringComparer::OrdinalIgnoreCase());
    CHECK(ToString(reader->Files().at("Game/fileA.uasset")->Read()) == kFileAText);
    CHECK(ToString(reader->Read(kChunkBulk)) == kBulkText);
}

static void TestProviderIntegration()
{
    RegisterFakeCodec();

    TestProvider provider;
    const auto ucas = MakeUcas({});
    provider.RegisterVfs(Archive("pakchunk0.utoc", MakeUtoc({})),
                         [ucas](const std::string& path) { return Archive(path, ucas); });
    CHECK(provider.UnloadedVfs().size() == 1);
    CHECK(provider.Mount() == 1);

    CHECK(ToString(provider.SaveAsset("Game/fileA.uasset")) == kFileAText);
    CHECK(ToString(provider.SaveAsset("Game/sub/fileB.bin")) == kFileBText);

    // ById holds the package (not its payload) under the chunk's package id.
    CHECK(provider.Files.ById().size() == 1);
    CHECK(provider.Files.ById().count(FPackageId(0x1111)) == 1);

    // FindPayloads: the cooked-index lookup walks the reader's files by chunk id.
    const auto pkg = provider.GetGameFile("Game/fileA.uasset");
    std::shared_ptr<CUE4Parse::FileProvider::Objects::GameFile> uexp;
    std::vector<std::shared_ptr<CUE4Parse::FileProvider::Objects::GameFile>> ubulks, uptnls;
    provider.Files.FindPayloads(*pkg, uexp, ubulks, uptnls, /*cookedIndexLookup*/ true);
    CHECK(uexp == nullptr);
    CHECK(ubulks.size() == 1 && ToString(ubulks[0]->Read()) == kBulkText);
    CHECK(uptnls.empty());

    // ...and the plain lookup finds the sibling ubulk by path just the same.
    provider.Files.FindPayloads(*pkg, uexp, ubulks, uptnls);
    CHECK(ubulks.size() == 1 && ToString(ubulks[0]->Read()) == kBulkText);

    // Loading a Zen package is an explicit gap, not a misparse.
    CHECK(provider.TryLoadPackage("Game/fileA.uasset") == nullptr);
    bool threw = false;
    try { provider.LoadPackage("Game/fileA.uasset"); }
    catch (const CUE4Parse::UE4::Exceptions::ParserException&) { threw = true; }
    CHECK(threw);
}

static void TestEncryptedIoStore()
{
    RegisterFakeCodec();

    TocOptions opt;
    opt.Encrypted = true;
    const auto key = std::make_shared<FAesKey>(std::vector<uint8_t>(32, 0x11));

    // No hook, wrong real AES key: the index-aligned probe decrypts to garbage and the reader never
    // mounts.
    {
        TestProvider provider;
        const auto ucas = MakeUcas(opt);
        provider.RegisterVfs(Archive("enc.utoc", MakeUtoc(opt)),
                             [ucas](const std::string& path) { return Archive(path, ucas); });
        CHECK(provider.UnloadedVfs().size() == 1);
        CHECK(provider.RequiredKeys().size() == 1);
        CHECK(provider.Mount() == 0);
        CHECK(provider.SubmitKey(FGuid(), key) == 0);
        CHECK(provider.MountedVfs().empty());
    }

    // With the XOR hook the same container mounts and every read decrypts.
    {
        TestProvider provider;
        provider.CustomEncryption = [&opt](const std::vector<uint8_t>& bytes, int beginOffset, int count,
                                           bool /*isIndex*/, IAesVfsReader&)
        {
            std::vector<uint8_t> out = bytes;
            for (int i = 0; i < count; ++i) out[static_cast<size_t>(beginOffset) + i] ^= opt.XorMask;
            return out;
        };
        const auto ucas = MakeUcas(opt);
        provider.RegisterVfs(Archive("enc.utoc", MakeUtoc(opt)),
                             [ucas](const std::string& path) { return Archive(path, ucas); });
        CHECK(provider.SubmitKey(FGuid(), key) == 1);
        CHECK(provider.MountedVfs().size() == 1);
        CHECK(provider.Keys().size() == 1);
        CHECK(ToString(provider.SaveAsset("Game/fileA.uasset")) == kFileAText);
        CHECK(ToString(provider.SaveAsset("Game/sub/fileB.bin")) == kFileBText);
    }
}

int main()
{
    TestTocParsing();
    TestBadMagic();
    TestMissingUcas();
    TestMountAndExtract();
    TestPartitionedUcas();
    TestPerfectHashResolution();
    TestProviderIntegration();
    TestEncryptedIoStore();

    if (g_failures == 0) std::printf("test_io_store: all checks passed\n");
    else std::printf("test_io_store: %d check(s) failed\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
