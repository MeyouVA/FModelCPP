// End-to-end tests for the pak layer: FPakInfo (trailer discovery), FPakEntry (its record formats), and
// PakFileReader (mounting + extraction).
//
// Every test builds a real pak byte-for-byte in memory and then mounts it through the actual reader, so the
// writer here doubles as an executable specification of the format. That matters more than usual for this
// layer: almost none of it can be checked by inspection, because a one-field slip in a record still parses
// and just silently yields the wrong bytes.
//
// Two things are exercised through stand-ins rather than the real thing:
//   * Compression, via a fake codec registered for the Oodle slot (no compressor ships with the port, so a
//     genuinely compressed pak cannot be authored here). The fake one tiles its input over the output,
//     which is enough to prove the block loop reads the right ranges and assembles them in order.
//   * Encryption, via the CustomEncryption delegate (a byte-wise XOR) rather than AES, because Aes only
//     decrypts. AES itself is pinned against published vectors in test_aes.cpp; what is checked here is the
//     decrypt *plumbing* — index vs data, alignment, the isIndex flag — plus that a missing or wrong AES
//     key is rejected rather than quietly producing garbage.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "Compression/Compression.h"
#include "Encryption/Aes/Aes.h"
#include "UE4/Exceptions/InvalidAesKeyException.h"
#include "UE4/Pak/PakFileReader.h"
#include "UE4/Readers/FByteArchive.h"

using namespace CUE4Parse::UE4::Pak;
using namespace CUE4Parse::UE4::Pak::Objects;
using CUE4Parse::Compression::CompressionAlgorithm;
using CUE4Parse::Compression::CompressionMethod;
using CUE4Parse::Encryption::Aes::FAesKey;
using CUE4Parse::UE4::Readers::FArchive;
using CUE4Parse::UE4::Readers::FByteArchive;
using CUE4Parse::Utils::StringComparer;

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL: %s (line %d)\n", #cond, __LINE__); ++g_failures; } } while (0)

// --------------------------------------------------------------------------------------------------
// A little-endian byte writer, so a pak can be spelled out in the order the format defines it.
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

    void Raw(const std::vector<uint8_t>& data) { Bytes.insert(Bytes.end(), data.begin(), data.end()); }
    void Raw(const std::string& data) { Bytes.insert(Bytes.end(), data.begin(), data.end()); }
    void Zeros(int64_t count) { Bytes.insert(Bytes.end(), static_cast<size_t>(count), 0); }

    // FString: a positive length counts ANSI chars including the null terminator.
    void FString(const std::string& s)
    {
        Put<int32_t>(static_cast<int32_t>(s.size() + 1));
        Raw(s);
        Put<uint8_t>(0);
    }
};

static std::vector<uint8_t> Xor(std::vector<uint8_t> data, uint8_t mask)
{
    for (uint8_t& b : data) b ^= mask;
    return data;
}

static std::string ToString(const std::vector<uint8_t>& bytes)
{
    return std::string(bytes.begin(), bytes.end());
}

static std::shared_ptr<FArchive> Archive(std::string name, std::vector<uint8_t> bytes)
{
    return std::make_shared<FByteArchive>(std::move(name), std::move(bytes));
}

// The five 32-byte compression-method names in the trailer. FPakInfo prepends None, so the resulting table
// is [None, Zlib, Gzip, Oodle] and Oodle lands at index 3.
static void WriteCompressionMethodNames(Writer& w)
{
    const char* names[5] = {"Zlib", "Gzip", "Oodle", "", ""};
    for (const char* name : names)
    {
        const size_t len = std::strlen(name);
        w.Raw(std::string(name));
        w.Zeros(static_cast<int64_t>(32 - len));
    }
}

// The Size8a trailer: 16 + 1 + 4 + 4 + 8 + 8 + 20 + 5*32 = 221 bytes, which is the first candidate size
// FPakInfo tries, so a correctly written one is found on the first probe.
static void WriteTrailer(Writer& w, int32_t version, int64_t indexOffset, int64_t indexSize, bool encryptedIndex)
{
    const int64_t start = w.Pos();
    w.Zeros(16);                                  // EncryptionKeyGuid
    w.Put<uint8_t>(encryptedIndex ? 1 : 0);
    w.Put<uint32_t>(FPakInfo::PAK_FILE_MAGIC);
    w.Put<int32_t>(version);
    w.Put<int64_t>(indexOffset);
    w.Put<int64_t>(indexSize);
    w.Zeros(20);                                  // IndexHash
    WriteCompressionMethodNames(w);
    CHECK(w.Pos() - start == 221);
}

// A legacy FPakEntry record. Blocks are relative to the entry's own Offset — that is what
// PakFile_Version_RelativeChunkOffsets means, and the reader adds Offset back on.
struct LegacyRecord
{
    int64_t Offset = 0;
    int64_t CompressedSize = 0;
    int64_t UncompressedSize = 0;
    int32_t MethodIndex = 0;
    std::vector<std::pair<int64_t, int64_t>> Blocks;
    uint8_t Flags = 0;
    uint32_t CompressionBlockSize = 0;

    // What FPakEntry computes as StructSize after reading this record, and therefore how many bytes of
    // duplicated record sit in front of the file's data.
    int32_t StructSize() const
    {
        int32_t size = 8 + 8 + 8 + 4 + 20 + 1 + 4;
        if (MethodIndex != 0) size += 4 + static_cast<int32_t>(Blocks.size()) * 16;
        return size;
    }

    void Write(Writer& w) const
    {
        const int64_t start = w.Pos();
        w.Put<int64_t>(Offset);
        w.Put<int64_t>(CompressedSize);
        w.Put<int64_t>(UncompressedSize);
        w.Put<int32_t>(MethodIndex);
        w.Zeros(20); // hash
        if (MethodIndex != 0)
        {
            w.Put<int32_t>(static_cast<int32_t>(Blocks.size()));
            for (const auto& b : Blocks) { w.Put<int64_t>(b.first); w.Put<int64_t>(b.second); }
        }
        w.Put<uint8_t>(Flags);
        w.Put<uint32_t>(CompressionBlockSize);
        CHECK(w.Pos() - start == StructSize());
    }
};

// The fake codec: tile the compressed bytes over the output buffer. Registered for Oodle, which has no real
// decompressor in this port, so nothing else can be affected by it.
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

static void TestCompressedBlock()
{
    const FPakCompressedBlock block(100, 164);
    CHECK(block.Size() == 64);
    CHECK(block.ToString() == "From 100 To 164 (=64)");
    CHECK(FPakCompressedBlock().Size() == 0);
}

static void TestTrailerNotFound()
{
    // A buffer with no recognisable trailer at any candidate offset must be reported, not guessed at.
    auto ar = Archive("junk.pak", std::vector<uint8_t>(512, 0x7F));
    bool threw = false;
    try { PakFileReader reader(ar); }
    catch (const CUE4Parse::UE4::Exceptions::ParserException&) { threw = true; }
    CHECK(threw);
}

// --------------------------------------------------------------------------------------------------
// Version 8, legacy index: one stored file and one "compressed" file split over three blocks.
// --------------------------------------------------------------------------------------------------
static void TestLegacyIndex()
{
    RegisterFakeCodec();

    const std::string plainText = "Hello, pak!";
    // Each block is its tile repeated to fill the block: 16 + 16 + 8 = 40 uncompressed bytes from 20 stored.
    const std::string tile0 = "ABCDEFGH", tile1 = "IJKLMNOP", tile2 = "QRST";
    const std::string expectedCompressed = tile0 + tile0 + tile1 + tile1 + tile2 + tile2;
    CHECK(expectedCompressed.size() == 40);

    LegacyRecord recA;
    recA.UncompressedSize = static_cast<int64_t>(plainText.size());
    recA.CompressedSize = recA.UncompressedSize;

    LegacyRecord recB;
    recB.MethodIndex = 3; // Oodle
    recB.UncompressedSize = 40;
    recB.CompressedSize = 20;
    recB.CompressionBlockSize = 16;
    const int32_t structB = 8 + 8 + 8 + 4 + 20 + 4 + 3 * 16 + 1 + 4;
    recB.Blocks = {{structB, structB + 8}, {structB + 8, structB + 16}, {structB + 16, structB + 20}};
    CHECK(recB.StructSize() == structB);

    Writer file;
    // --- file data region ---
    recA.Offset = file.Pos();
    file.Zeros(recA.StructSize()); // the duplicated record in front of the payload
    file.Raw(plainText);

    recB.Offset = file.Pos();
    file.Zeros(recB.StructSize());
    file.Raw(tile0);
    file.Raw(tile1);
    file.Raw(tile2);

    // --- index ---
    Writer index;
    index.FString("../../../Game/");
    index.Put<int32_t>(2);
    index.FString("fileA.uasset");
    recA.Write(index);
    index.FString("sub/fileB.bin");
    recB.Write(index);

    const int64_t indexOffset = file.Pos();
    file.Raw(index.Bytes);
    WriteTrailer(file, 8, indexOffset, index.Pos(), false);

    // --- mount ---
    PakFileReader reader(Archive("pakchunk0-Windows.pak", file.Bytes));
    CHECK(reader.Info.Magic == FPakInfo::PAK_FILE_MAGIC);
    CHECK(reader.Info.Version == EPakFileVersion::PakFile_Version_FNameBasedCompressionMethod);
    // Version 8 found at the Size8a offset is the UE4.23 layout, which is what IsSubVersion records.
    CHECK(reader.Info.IsSubVersion);
    CHECK(!reader.Info.EncryptedIndex);
    CHECK(reader.Info.IndexOffset == indexOffset);
    CHECK(reader.Info.CompressionMethods.size() == 4);
    CHECK(reader.Info.CompressionMethods[0] == CompressionMethod::None);
    CHECK(reader.Info.CompressionMethods[3] == CompressionMethod::Oodle);
    CHECK(!reader.IsEncrypted());

    reader.Mount(StringComparer::OrdinalIgnoreCase());
    CHECK(reader.MountPoint() == "Game/");
    CHECK(reader.FileCount() == 2);
    CHECK(reader.ReadOrder() == 3); // no _P suffix, so no patch priority

    const auto& files = reader.Files();
    CHECK(files.count("Game/fileA.uasset") == 1);
    CHECK(files.count("Game/sub/fileB.bin") == 1);
    // The map was built with the comparer Mount was handed.
    CHECK(files.count("GAME/FILEA.UASSET") == 1);

    auto* entryA = dynamic_cast<FPakEntry*>(files.at("Game/fileA.uasset").get());
    CHECK(entryA != nullptr);
    CHECK(entryA->Offset == recA.Offset);
    CHECK(entryA->Size == static_cast<int64_t>(plainText.size()));
    CHECK(entryA->StructSize == recA.StructSize());
    CHECK(entryA->GetCompressionMethod() == CompressionMethod::None);
    CHECK(!entryA->IsCompressed());
    CHECK(!entryA->IsEncrypted());
    CHECK(!entryA->IsDeleted());
    CHECK(entryA->Name() == "fileA.uasset");
    CHECK(entryA->Extension() == "uasset");
    CHECK(entryA->IsUePackage());
    CHECK(ToString(entryA->Read()) == plainText);

    auto* entryB = dynamic_cast<FPakEntry*>(files.at("Game/sub/fileB.bin").get());
    CHECK(entryB != nullptr);
    CHECK(entryB->IsCompressed());
    CHECK(entryB->GetCompressionMethod() == CompressionMethod::Oodle);
    CHECK(entryB->CompressionBlocks.size() == 3);
    CHECK(entryB->StructSize == structB);
    // Relative block offsets have been rebased onto the entry.
    CHECK(entryB->CompressionBlocks[0].CompressedStart == recB.Offset + structB);
    CHECK(entryB->CompressionBlocks[2].CompressedEnd == recB.Offset + structB + 20);
    CHECK(entryB->Directory() == "Game/sub");
    CHECK(!entryB->IsUePackage());
    CHECK(ToString(entryB->Read()) == expectedCompressed);

    // CreateReader hands back an archive over exactly those bytes.
    auto sub = entryB->CreateReader();
    CHECK(sub->Length == 40);
    CHECK(sub->Name() == "Game/sub/fileB.bin");

    // A patch pak sorts ahead of the base one.
    PakFileReader patch(Archive("pakchunk0-Windows_1_P.pak", file.Bytes));
    patch.Mount(StringComparer::Ordinal());
    CHECK(patch.ReadOrder() == 203); // 3 + 100 * (1 + 1)
}

// --------------------------------------------------------------------------------------------------
// Version 11, updated index: a bit-packed "encoded" entry plus a non-encoded one, reached through the
// directory index.
// --------------------------------------------------------------------------------------------------
static void TestUpdatedIndex()
{
    const std::string encodedText = "encoded payload";
    const std::string plainText = "non-encoded payload";
    const int32_t structSize = 8 + 8 + 8 + 4 + 20 + 1 + 4; // both entries are stored, so both are 53 bytes

    Writer file;
    const int64_t encodedOffset = file.Pos();
    file.Zeros(structSize);
    file.Raw(encodedText);

    const int64_t plainOffset = file.Pos();
    file.Zeros(structSize);
    file.Raw(plainText);

    // --- the encoded-entries blob: one bitfield-packed record ---
    Writer encoded;
    // bit31 = offset fits in 32 bits, bit30 = uncompressed size fits in 32 bits, method 0, no blocks.
    encoded.Put<uint32_t>((1u << 31) | (1u << 30));
    encoded.Put<uint32_t>(static_cast<uint32_t>(encodedOffset));
    encoded.Put<uint32_t>(static_cast<uint32_t>(encodedText.size()));

    // --- the non-encoded entries, written in the legacy record form ---
    LegacyRecord plainRecord;
    plainRecord.Offset = plainOffset;
    plainRecord.UncompressedSize = static_cast<int64_t>(plainText.size());
    plainRecord.CompressedSize = plainRecord.UncompressedSize;

    // --- directory index (its own region in the file) ---
    Writer directory;
    directory.Put<int32_t>(1);          // one directory
    directory.FString("/");             // ...the root, which is trimmed against the mount point
    directory.Put<int32_t>(2);          // two files in it
    directory.FString("encoded.uasset");
    directory.Put<int32_t>(0);          // >= 0 : an offset into the encoded blob
    directory.FString("plain.bin");
    directory.Put<int32_t>(-1);         // < 0 : -(index + 1) into the non-encoded array

    const int64_t directoryOffset = file.Pos();
    file.Raw(directory.Bytes);

    // --- primary index ---
    Writer primary;
    primary.FString("../../../Game/");
    primary.Put<int32_t>(2);            // file count
    primary.Zeros(8);                   // PathHashSeed
    primary.Put<int32_t>(1);            // has path hash index
    primary.Zeros(36);                  // its offset/size/hash
    primary.Put<int32_t>(1);            // has directory index
    primary.Put<int64_t>(directoryOffset);
    primary.Put<int64_t>(directory.Pos());
    primary.Zeros(20);                  // directory index hash
    primary.Put<int32_t>(static_cast<int32_t>(encoded.Pos()));
    primary.Raw(encoded.Bytes);
    primary.Put<int32_t>(1);            // one non-encoded entry
    plainRecord.Write(primary);

    const int64_t indexOffset = file.Pos();
    file.Raw(primary.Bytes);
    WriteTrailer(file, 11, indexOffset, primary.Pos(), false);

    PakFileReader reader(Archive("updated.pak", file.Bytes));
    CHECK(reader.Info.Version == EPakFileVersion::PakFile_Version_Fnv64BugFix);
    reader.Mount(StringComparer::Ordinal());

    CHECK(reader.MountPoint() == "Game/");
    CHECK(reader.FileCount() == 2);

    const auto& files = reader.Files();
    auto* encodedEntry = dynamic_cast<FPakEntry*>(files.at("Game/encoded.uasset").get());
    CHECK(encodedEntry != nullptr);
    CHECK(encodedEntry->Offset == encodedOffset);
    CHECK(encodedEntry->UncompressedSize == static_cast<int64_t>(encodedText.size()));
    CHECK(encodedEntry->CompressedSize == encodedEntry->UncompressedSize);
    CHECK(encodedEntry->StructSize == structSize);
    CHECK(encodedEntry->CompressionBlocks.empty());
    CHECK(!encodedEntry->IsEncrypted());
    CHECK(ToString(encodedEntry->Read()) == encodedText);

    auto* plainEntry = dynamic_cast<FPakEntry*>(files.at("Game/plain.bin").get());
    CHECK(plainEntry != nullptr);
    CHECK(plainEntry->Offset == plainOffset);
    // The non-encoded entry is read with an empty path and only gets its real one from the directory index.
    CHECK(plainEntry->Path() == "Game/plain.bin");
    CHECK(ToString(plainEntry->Read()) == plainText);

    // A case-sensitive comparer must not fold the two spellings together.
    CHECK(files.count("game/plain.bin") == 0);
}

// --------------------------------------------------------------------------------------------------
// Encryption plumbing.
// --------------------------------------------------------------------------------------------------
static void TestEncryptedPak()
{
    constexpr uint8_t mask = 0x5A;
    const std::string plainText = "encrypted payload";

    LegacyRecord rec;
    rec.UncompressedSize = static_cast<int64_t>(plainText.size());
    rec.CompressedSize = rec.UncompressedSize;
    rec.Flags = 1; // Flag_Encrypted

    Writer file;
    rec.Offset = file.Pos();
    file.Zeros(rec.StructSize());
    {
        // The data region is read back 16-aligned, so it has to be padded out to a whole block.
        std::vector<uint8_t> payload(plainText.begin(), plainText.end());
        payload.resize(32, 0);
        file.Raw(Xor(payload, mask));
    }

    Writer index;
    index.FString("../../../Game/");
    index.Put<int32_t>(1);
    index.FString("secret.bin");
    rec.Write(index);

    const int64_t indexOffset = file.Pos();
    const std::vector<uint8_t> encryptedIndex = Xor(index.Bytes, mask);
    file.Raw(encryptedIndex);
    WriteTrailer(file, 8, indexOffset, static_cast<int64_t>(encryptedIndex.size()), true);

    // --- with the custom decryption hook wired up ---
    {
        PakFileReader reader(Archive("secret.pak", file.Bytes));
        CHECK(reader.IsEncrypted());

        bool sawIndexCall = false;
        reader.CustomEncryption() = [&](const std::vector<uint8_t>& bytes, int beginOffset, int count,
                                        bool isIndex, CUE4Parse::UE4::VirtualFileSystem::IAesVfsReader&) {
            if (isIndex) sawIndexCall = true;
            std::vector<uint8_t> slice(bytes.begin() + beginOffset, bytes.begin() + beginOffset + count);
            return Xor(std::move(slice), mask);
        };

        reader.Mount(StringComparer::Ordinal());
        CHECK(sawIndexCall); // the index is decrypted with isIndex = true, the payload with false
        CHECK(reader.FileCount() == 1);
        CHECK(reader.EncryptedFileCount() == 1);

        auto* entry = dynamic_cast<FPakEntry*>(reader.Files().at("Game/secret.bin").get());
        CHECK(entry != nullptr);
        CHECK(entry->IsEncrypted());
        // The read is padded up to the AES block size but the entry's own size is what comes back.
        CHECK(ToString(entry->Read()) == plainText);
    }

    // --- without a key, and with the wrong key ---
    {
        PakFileReader reader(Archive("secret.pak", file.Bytes));
        bool threw = false;
        try { reader.Mount(StringComparer::Ordinal()); }
        catch (const CUE4Parse::UE4::Exceptions::InvalidAesKeyException&) { threw = true; }
        CHECK(threw);
    }
    {
        PakFileReader reader(Archive("secret.pak", file.Bytes));
        // A real AES key that does not decrypt this pak: the mount-point probe rejects it rather than
        // letting the garbage through.
        reader.AesKey() = std::make_shared<FAesKey>(std::vector<uint8_t>(32, 0x42));
        CHECK(!reader.TestAesKey(*reader.AesKey()));
        bool threw = false;
        try { reader.Mount(StringComparer::Ordinal()); }
        catch (const CUE4Parse::UE4::Exceptions::InvalidAesKeyException&) { threw = true; }
        CHECK(threw);
    }
}

// --------------------------------------------------------------------------------------------------
// The mount-point normaliser and the index probe, which decide whether an AES key is accepted.
// --------------------------------------------------------------------------------------------------
static void TestIsValidIndex()
{
    using CUE4Parse::UE4::VirtualFileSystem::AbstractVfsReader;

    Writer good;
    good.FString("../../../Game/");
    CHECK(AbstractVfsReader::IsValidIndex(good.Bytes));

    // A wide (UCS-2) mount point is equally valid. The probe's seek arithmetic overshoots the terminator by
    // two code units (a C# quirk this port keeps), so it inspects position 12 rather than 8 — harmless
    // against the zero-padded 260-byte buffer MountPointCheckBytes actually hands it, modelled here by the
    // trailing zeros.
    Writer wide;
    wide.Put<int32_t>(-3);
    wide.Put<uint16_t>('/');
    wide.Put<uint16_t>('a');
    wide.Put<uint16_t>(0);
    wide.Zeros(16);
    CHECK(AbstractVfsReader::IsValidIndex(wide.Bytes));

    // ...and a non-zero at that spot is what makes the probe reject a wrongly-decrypted buffer.
    Writer wideBad;
    wideBad.Put<int32_t>(-3);
    wideBad.Put<uint16_t>('/');
    wideBad.Put<uint16_t>('a');
    wideBad.Put<uint16_t>(0);
    wideBad.Zeros(2);                 // brings the cursor to offset 12, where the probe looks
    wideBad.Put<uint16_t>(0x1234);
    wideBad.Zeros(8);
    CHECK(!AbstractVfsReader::IsValidIndex(wideBad.Bytes));

    // An implausible length is rejected outright...
    Writer tooLong;
    tooLong.Put<int32_t>(9999);
    tooLong.Zeros(64);
    CHECK(!AbstractVfsReader::IsValidIndex(tooLong.Bytes));

    // ...as is a plausible length whose terminator is not actually zero.
    Writer unterminated;
    unterminated.Put<int32_t>(4);
    unterminated.Raw(std::string("abcd"));
    CHECK(!AbstractVfsReader::IsValidIndex(unterminated.Bytes));

    // A length that runs off the end must answer "no", not throw — this runs on wrongly-decrypted bytes.
    Writer truncated;
    truncated.Put<int32_t>(100);
    truncated.Zeros(4);
    CHECK(!AbstractVfsReader::IsValidIndex(truncated.Bytes));

    CHECK(AbstractVfsReader::IsValidIndex(std::vector<uint8_t>(8, 0))); // length 0, then a zero byte
}

static void TestMountPointFallback()
{
    // A mount point that does not start with ../../.. is replaced by the root rather than trusted.
    Writer file;
    LegacyRecord rec;
    rec.Offset = file.Pos();
    file.Zeros(rec.StructSize());

    Writer index;
    index.FString("/Weird/Mount");
    index.Put<int32_t>(1);
    index.FString("thing.bin");
    rec.Write(index);

    const int64_t indexOffset = file.Pos();
    file.Raw(index.Bytes);
    WriteTrailer(file, 8, indexOffset, index.Pos(), false);

    PakFileReader reader(Archive("weird.pak", file.Bytes));
    reader.Mount(StringComparer::Ordinal());
    CHECK(reader.MountPoint().empty()); // "/" with its leading slash stripped
    CHECK(reader.Files().count("thing.bin") == 1);
}

static void TestDeletedEntriesAreSkipped()
{
    Writer file;
    LegacyRecord live, deleted;
    live.Offset = file.Pos();
    file.Zeros(live.StructSize());
    file.Raw(std::string("data"));
    live.UncompressedSize = 4;
    live.CompressedSize = 4;

    deleted.Flags = 2; // Flag_Deleted, with a zero size
    deleted.Offset = 0;

    Writer index;
    index.FString("../../../Game/");
    index.Put<int32_t>(2);
    index.FString("live.bin");
    live.Write(index);
    index.FString("gone.bin");
    deleted.Write(index);

    const int64_t indexOffset = file.Pos();
    file.Raw(index.Bytes);
    WriteTrailer(file, 8, indexOffset, index.Pos(), false);

    PakFileReader reader(Archive("deletes.pak", file.Bytes));
    reader.Mount(StringComparer::Ordinal());
    CHECK(reader.FileCount() == 1);
    CHECK(reader.Files().count("Game/live.bin") == 1);
    CHECK(reader.Files().count("Game/gone.bin") == 0);
}

int main()
{
    TestCompressedBlock();
    TestTrailerNotFound();
    TestLegacyIndex();
    TestUpdatedIndex();
    TestEncryptedPak();
    TestIsValidIndex();
    TestMountPointFallback();
    TestDeletedEntriesAreSkipped();

    if (g_failures == 0) std::printf("test_pak: all checks passed\n");
    else std::printf("test_pak: %d check(s) failed\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
