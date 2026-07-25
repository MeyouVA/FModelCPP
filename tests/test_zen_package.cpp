// End-to-end tests for the mappings + Zen (IO Store asset) slice:
//   * UsmapParser / UsmapTypeMappingsProvider over an authored .usmap blob
//   * FUnversionedHeader + FIterator over authored fragment/zero-mask bytes
//   * IoPackage read through a real provider: a global.utoc supplying IoGlobalData (global name batch +
//     script objects) and a package .utoc holding an FIoContainerHeader chunk plus the Zen package chunk,
//     whose single export deserializes its unversioned properties through the usmap mappings.
//
// Every fixture is authored byte-for-byte and goes through the actual readers, like test_io_store.cpp.
// The package is a UE5.0 Zen package (FZenPackageSummary path, export-bundle headers, no bulk-data map).
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "FileProvider/Vfs/AbstractVfsFileProvider.h"
#include "MappingsProvider/Usmap/UsmapParser.h"
#include "MappingsProvider/Usmap/UsmapTypeMappingsProvider.h"
#include "UE4/Assets/IoPackage.h"
#include "UE4/Assets/Objects/Properties/FloatProperty.h"
#include "UE4/Assets/Objects/Properties/IntProperty.h"
#include "UE4/Assets/Objects/Unversioned/FIterator.h"
#include "UE4/Assets/Objects/Unversioned/FUnversionedHeader.h"
#include "UE4/Exceptions/ParserException.h"
#include "UE4/IO/Objects/FIoChunkId.h"
#include "UE4/IO/Objects/FIoStoreTocResource.h"
#include "UE4/IO/Objects/FPackageId.h"
#include "UE4/IO/Objects/FPackageObjectIndex.h"
#include "UE4/Readers/FByteArchive.h"
#include "UE4/Versions/EGame.h"

using namespace CUE4Parse::UE4::IO::Objects;
using CUE4Parse::FileProvider::Vfs::AbstractVfsFileProvider;
using CUE4Parse::MappingsProvider::Usmap::EPropertyType;
using CUE4Parse::MappingsProvider::Usmap::EUsmapVersion;
using CUE4Parse::MappingsProvider::Usmap::UsmapTypeMappingsProvider;
using CUE4Parse::UE4::Assets::IoPackage;
using CUE4Parse::UE4::Assets::Objects::Properties::FloatProperty;
using CUE4Parse::UE4::Assets::Objects::Properties::IntProperty;
using CUE4Parse::UE4::Assets::Objects::Unversioned::FIterator;
using CUE4Parse::UE4::Assets::Objects::Unversioned::FUnversionedHeader;
using CUE4Parse::UE4::Readers::FArchive;
using CUE4Parse::UE4::Readers::FByteArchive;
using CUE4Parse::UE4::Versions::VersionContainer;
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

    void Put5BE(uint64_t v)
    {
        Put<uint8_t>(static_cast<uint8_t>(v >> 32));
        Put<uint8_t>(static_cast<uint8_t>(v >> 24));
        Put<uint8_t>(static_cast<uint8_t>(v >> 16));
        Put<uint8_t>(static_cast<uint8_t>(v >> 8));
        Put<uint8_t>(static_cast<uint8_t>(v));
    }

    // An IO Store "name batch" blob: count, string-byte count, hash version, hashes, headers, strings.
    void NameBatch(const std::vector<std::string>& names)
    {
        Put<int32_t>(static_cast<int32_t>(names.size()));
        if (names.empty()) return;
        uint32_t stringBytes = 0;
        for (const auto& n : names) stringBytes += static_cast<uint32_t>(n.size());
        Put<uint32_t>(stringBytes);
        Put<uint64_t>(0xC1640000ull); // hash version (skipped by the loader)
        for (size_t i = 0; i < names.size(); i++) Put<uint64_t>(0); // hashes (skipped)
        for (const auto& n : names)
        {
            Put<uint8_t>(static_cast<uint8_t>((n.size() >> 8) & 0x7F)); // 8-bit (not utf16)
            Put<uint8_t>(static_cast<uint8_t>(n.size() & 0xFF));
        }
        for (const auto& n : names) Raw(n);
    }
};

static std::shared_ptr<FArchive> Archive(std::string name, std::vector<uint8_t> bytes)
{
    return std::make_shared<FByteArchive>(std::move(name), std::move(bytes));
}

// --------------------------------------------------------------------------------------------------
// Part 1: the .usmap fixture — one struct "TestStruct" with three properties.
// --------------------------------------------------------------------------------------------------
static const std::vector<std::string> kUsmapNames = {"TestStruct", "IntProp", "FloatProp", "ZeroProp"};

static std::vector<uint8_t> MakeUsmap()
{
    Writer body;
    body.Put<uint32_t>(static_cast<uint32_t>(kUsmapNames.size()));
    for (const auto& n : kUsmapNames)
    {
        body.Put<uint16_t>(static_cast<uint16_t>(n.size())); // LongFName: 16-bit lengths
        body.Raw(n);
    }
    body.Put<uint32_t>(0); // enum count

    body.Put<uint32_t>(1); // struct count
    body.Put<int32_t>(0);  // name  -> "TestStruct"
    body.Put<int32_t>(-1); // super -> none
    body.Put<uint16_t>(3); // property count
    body.Put<uint16_t>(3); // serializable property count
    const auto prop = [&body](uint16_t index, uint8_t arrayDim, int32_t nameIdx, EPropertyType type)
    {
        body.Put<uint16_t>(index);
        body.Put<uint8_t>(arrayDim);
        body.Put<int32_t>(nameIdx);
        body.Put<uint8_t>(static_cast<uint8_t>(type));
    };
    prop(0, 1, 1, EPropertyType::IntProperty);
    prop(1, 1, 2, EPropertyType::FloatProperty);
    prop(2, 1, 3, EPropertyType::IntProperty);

    Writer usmap;
    usmap.Put<uint16_t>(0x30C4); // FileMagic
    usmap.Put<uint8_t>(static_cast<uint8_t>(EUsmapVersion::Latest));
    usmap.Put<int32_t>(0);       // bHasVersioning (FArchive::ReadBoolean reads an int32, as in C#)
    usmap.Put<uint8_t>(0);       // EUsmapCompressionMethod::None
    usmap.Put<uint32_t>(static_cast<uint32_t>(body.Pos())); // compressed size
    usmap.Put<uint32_t>(static_cast<uint32_t>(body.Pos())); // decompressed size
    usmap.Raw(body.Bytes);
    return usmap.Bytes;
}

// UsmapTypeMappingsProvider is abstract (Reload is the file provider's job, as in C#); these tests load
// from memory, so they need a concrete no-op Reload.
class MemoryUsmapProvider : public UsmapTypeMappingsProvider
{
public:
    void Reload() override {}
};

static void TestUsmapParsing()
{
    MemoryUsmapProvider provider;
    provider.Load(MakeUsmap());

    const auto* mappings = provider.MappingsForGame();
    CHECK(mappings != nullptr);
    if (mappings == nullptr) return;

    CHECK(mappings->Types.size() == 1);
    const auto it = mappings->Types.find("TestStruct");
    CHECK(it != mappings->Types.end());
    if (it == mappings->Types.end()) return;

    const auto& s = *it->second;
    CHECK(s.Name == "TestStruct");
    CHECK(!s.SuperType.has_value());
    CHECK(s.PropertyCount == 3);
    CHECK(s.Properties.size() == 3);

    const auto* p0 = s.TryGetValue(0);
    const auto* p1 = s.TryGetValue(1);
    const auto* p2 = s.TryGetValue(2);
    CHECK(p0 != nullptr && p0->Name == "IntProp" && p0->MappingType->Type == "IntProperty");
    CHECK(p1 != nullptr && p1->Name == "FloatProp" && p1->MappingType->Type == "FloatProperty");
    CHECK(p2 != nullptr && p2->Name == "ZeroProp" && p2->MappingType->Type == "IntProperty");
    CHECK(s.TryGetValue(3) == nullptr);
    // The name lookup is case-insensitive by default (C#'s StringComparer.OrdinalIgnoreCase).
    CHECK(mappings->Types.find("teststruct") != mappings->Types.end());

    // A truncated / mistyped file is rejected rather than misparsed.
    auto bad = MakeUsmap();
    bad[0] ^= 0xFF;
    bool threw = false;
    try { MemoryUsmapProvider().Load(bad); }
    catch (const CUE4Parse::UE4::Exceptions::ParserException&) { threw = true; }
    CHECK(threw);
}

// --------------------------------------------------------------------------------------------------
// Part 2: the unversioned header / iterator.
// --------------------------------------------------------------------------------------------------
// Fragment bits: SkipNum 0x007f | HasZeroes 0x0080 | IsLast 0x0100 | ValueNum << 9.
static constexpr uint16_t Fragment(int skipNum, int valueNum, bool hasZeroes, bool isLast)
{
    return static_cast<uint16_t>((skipNum & 0x7F) | (hasZeroes ? 0x80 : 0) | (isLast ? 0x100 : 0) |
                                 (valueNum << 9));
}

static void TestUnversionedHeader()
{
    // One fragment, three values, the third of which is zero (zero-mask bit set).
    {
        Writer w;
        w.Put<uint16_t>(Fragment(0, 3, true, true));
        w.Put<uint8_t>(0x04); // bits: value0 non-zero, value1 non-zero, value2 zero
        FByteArchive ar("header", w.Bytes);

        const FUnversionedHeader header(ar);
        CHECK(header.Fragments.size() == 1);
        CHECK(header.HasValues());
        CHECK(header.HasNonZeroValues);
        CHECK(ar.Position == 3); // 2 fragment bytes + a 1-byte zero mask

        FIterator it(header);
        std::vector<std::pair<int, bool>> seen;
        do { seen.push_back(it.Current()); } while (it.MoveNext());
        CHECK(seen.size() == 3);
        CHECK(seen[0] == std::make_pair(0, true));
        CHECK(seen[1] == std::make_pair(1, true));
        CHECK(seen[2] == std::make_pair(2, false));
    }

    // Two fragments with a skip in between: schema indices 0,1 then (skip 2) 4.
    {
        Writer w;
        w.Put<uint16_t>(Fragment(0, 2, false, false));
        w.Put<uint16_t>(Fragment(2, 1, false, true));
        FByteArchive ar("header", w.Bytes);

        const FUnversionedHeader header(ar);
        CHECK(header.Fragments.size() == 2);
        CHECK(header.HasNonZeroValues);
        CHECK(ar.Position == 4); // no zero mask

        FIterator it(header);
        std::vector<int> indices;
        do { indices.push_back(it.Current().first); } while (it.MoveNext());
        CHECK(indices.size() == 3);
        CHECK(indices[0] == 0 && indices[1] == 1 && indices[2] == 4);
    }

    // An all-zero header: no values at all.
    {
        Writer w;
        w.Put<uint16_t>(Fragment(0, 0, false, true));
        FByteArchive ar("header", w.Bytes);
        const FUnversionedHeader header(ar);
        CHECK(!header.HasValues());
        CHECK(!header.HasNonZeroValues);
    }
}

// --------------------------------------------------------------------------------------------------
// Part 3: the IO Store containers.
// --------------------------------------------------------------------------------------------------
struct Chunk
{
    FIoChunkId Id;
    std::vector<uint8_t> Data;
};

struct Container
{
    std::vector<uint8_t> Toc;
    std::vector<uint8_t> Ucas;
};

static constexpr uint32_t kBlockSize = 0x10000; // every fixture chunk fits in one block

// Writes a .utoc/.ucas pair holding `chunks`, each stored uncompressed in its own block. When
// `indexedFile` is non-empty a directory index maps that path (under "../../../Game/") to chunk 0.
static Container BuildContainer(uint64_t containerId, const std::vector<Chunk>& chunks,
                                const std::string& indexedFile)
{
    Container out;

    Writer ucas;
    struct Block { uint64_t UcasOffset; uint32_t Size; };
    std::vector<Block> blocks;
    for (const auto& c : chunks)
    {
        blocks.push_back({static_cast<uint64_t>(ucas.Pos()), static_cast<uint32_t>(c.Data.size())});
        ucas.Raw(c.Data);
    }
    out.Ucas = ucas.Bytes;

    Writer index;
    if (!indexedFile.empty())
    {
        constexpr uint32_t INVALID = UINT32_MAX;
        index.FString("../../../Game/");
        index.Put<int32_t>(1); // one directory: the root
        index.Put<uint32_t>(INVALID); index.Put<uint32_t>(INVALID); index.Put<uint32_t>(INVALID); index.Put<uint32_t>(0);
        index.Put<int32_t>(1); // one file
        index.Put<uint32_t>(0); index.Put<uint32_t>(INVALID); index.Put<uint32_t>(0); // name 0, no next, toc index 0
        index.Put<int32_t>(1); // string table
        index.FString(indexedFile);
    }

    Writer toc;
    toc.Raw(std::vector<uint8_t>(FIoStoreTocHeader::TOC_MAGIC.begin(), FIoStoreTocHeader::TOC_MAGIC.end()));
    toc.Put<uint8_t>(3);   // EIoStoreTocVersion::PartitionSize
    toc.Zeros(3);          // reserved
    toc.Put<uint32_t>(144);                                     // TocHeaderSize
    toc.Put<uint32_t>(static_cast<uint32_t>(chunks.size()));    // TocEntryCount
    toc.Put<uint32_t>(static_cast<uint32_t>(blocks.size()));    // TocCompressedBlockEntryCount
    toc.Put<uint32_t>(12);                                      // TocCompressedBlockEntrySize
    toc.Put<uint32_t>(0);                                       // CompressionMethodNameCount (only None)
    toc.Put<uint32_t>(32);                                      // CompressionMethodNameLength
    toc.Put<uint32_t>(kBlockSize);                              // CompressionBlockSize
    toc.Put<uint32_t>(static_cast<uint32_t>(index.Pos()));      // DirectoryIndexSize
    toc.Put<uint32_t>(1);                                       // PartitionCount
    toc.Put<uint64_t>(containerId);
    toc.Zeros(16);                                              // EncryptionKeyGuid
    toc.Put<uint32_t>(index.Pos() > 0 ? 8u : 0u);               // ContainerFlags: Indexed
    toc.Put<uint32_t>(0);                                       // TocChunkPerfectHashSeedsCount
    toc.Put<uint64_t>(UINT64_MAX);                              // PartitionSize
    toc.Put<uint32_t>(0);                                       // TocChunksWithoutPerfectHashCount
    toc.Zeros(4 + 40);                                          // reserved

    for (const auto& c : chunks) toc.Put(c.Id);
    for (size_t i = 0; i < chunks.size(); i++)
    {
        toc.Put5BE(static_cast<uint64_t>(i) * kBlockSize); // virtual offset: one block per chunk
        toc.Put5BE(chunks[i].Data.size());
    }
    for (const auto& b : blocks)
    {
        toc.Put<uint64_t>(b.UcasOffset | (static_cast<uint64_t>(b.Size) << 40));
        toc.Put<uint32_t>(b.Size); // uncompressed size, method index 0 (None)
    }
    toc.Raw(index.Bytes);

    out.Toc = toc.Bytes;
    return out;
}

// --------------------------------------------------------------------------------------------------
// Part 4: the Zen package fixture.
// --------------------------------------------------------------------------------------------------
static const std::string kPackageName = "/Game/TestPackage";
static const std::string kExportName = "TestExport";
static const std::string kClassName = "TestStruct"; // matches the usmap struct
static constexpr uint64_t kContainerId = 0xC0FFEE;
// FPackageObjectIndex: 2 type bits + 62 index bits. EType::ScriptImport == 1.
static constexpr uint64_t kScriptClassIndex = (1ull << 62) | 0x42;

// The global container's ScriptObjects chunk: the global name batch plus one script object.
static std::vector<uint8_t> MakeScriptObjectsChunk()
{
    Writer w;
    w.NameBatch({kClassName});
    w.Put<int32_t>(1); // numScriptObjects
    // FScriptObjectEntry (32 bytes)
    w.Put<uint32_t>(0u | (2u << 30)); // ObjectName: global name pool, index 0
    w.Put<uint32_t>(0);               // ExtraIndex
    w.Put<uint64_t>(kScriptClassIndex);          // GlobalIndex
    w.Put<uint64_t>(FPackageObjectIndex::Invalid); // OuterIndex
    w.Put<uint64_t>(FPackageObjectIndex::Invalid); // CDOClassIndex
    return w.Bytes;
}

// The package container's FIoContainerHeader chunk (EIoContainerHeaderVersion::Initial).
static std::vector<uint8_t> MakeContainerHeaderChunk(FPackageId packageId)
{
    Writer w;
    w.Put<uint32_t>(0x496f436e); // signature
    w.Put<int32_t>(static_cast<int32_t>(EIoContainerHeaderVersion::Initial));
    w.Put<uint64_t>(kContainerId);
    w.Put<uint32_t>(1); // packageCount (unused before OptionalSegmentPackages)

    w.Put<int32_t>(1);  // PackageIds count
    w.Put<uint64_t>(packageId.id);
    w.Put<int32_t>(24); // storeEntriesSize: one 24-byte FFilePackageStoreEntry
    w.Put<int32_t>(1);  // ExportCount
    w.Put<int32_t>(1);  // ExportBundleCount
    w.Put<int32_t>(0);  // ImportedPackages: TCArrayView count
    w.Put<int32_t>(0);  // ImportedPackages: offset to data
    w.Put<uint64_t>(0); // ShaderMapHashes view (skipped unread)

    w.Put<int32_t>(0);  // ContainerNameMap: empty name batch
    w.Put<int32_t>(0);  // PackageRedirects count
    return w.Bytes;
}

// The Zen package itself: FZenPackageSummary, name batch, (no imports) export map, export-bundle
// entries, one export-bundle header, then the export's unversioned property data.
static std::vector<uint8_t> MakeZenPackage()
{
    // --- name batch + the rest of the header, laid out to compute the offsets the summary announces ---
    Writer names;
    names.NameBatch({kPackageName, kExportName});

    constexpr int summarySize = 44; // see FZenPackageSummary (UE5.0: GraphDataOffset, no dependency bundles)
    const int importedHashesOffset = summarySize + static_cast<int>(names.Pos());
    const int importMapOffset = importedHashesOffset;                 // no imported public export hashes
    const int exportMapOffset = importMapOffset;                      // no imports
    const int exportBundleEntriesOffset = exportMapOffset + 72;       // one FExportMapEntry
    const int graphDataOffset = exportBundleEntriesOffset + 2 * 8;    // ExportCount * 2 bundle entries
    const int headerSize = graphDataOffset + 16;                      // one FExportBundleHeader

    // --- the export's data: an unversioned header + two non-zero values ---
    Writer exportData;
    exportData.Put<uint16_t>(Fragment(0, 3, true, true));
    exportData.Put<uint8_t>(0x04); // ZeroProp (schema index 2) is stored as zero
    exportData.Put<int32_t>(42);   // IntProp
    exportData.Put<float>(1.5f);   // FloatProp

    Writer w;
    // FZenPackageSummary
    w.Put<uint32_t>(0);                     // bHasVersioningInfo
    w.Put<uint32_t>(headerSize);
    w.Put<uint32_t>(0);                     // Name: package name pool, index 0
    w.Put<uint32_t>(0);                     // Name.ExtraIndex
    w.Put<uint32_t>(0x00002000);            // PackageFlags: PKG_UnversionedProperties
    w.Put<uint32_t>(headerSize);            // CookedHeaderSize
    w.Put<int32_t>(importedHashesOffset);
    w.Put<int32_t>(importMapOffset);
    w.Put<int32_t>(exportMapOffset);
    w.Put<int32_t>(exportBundleEntriesOffset);
    w.Put<int32_t>(graphDataOffset);
    CHECK(w.Pos() == summarySize);

    w.Raw(names.Bytes);
    CHECK(w.Pos() == exportMapOffset);

    // FExportMapEntry (72 bytes)
    const int64_t exportStart = w.Pos();
    w.Put<uint64_t>(0);                                // CookedSerialOffset
    w.Put<uint64_t>(static_cast<uint64_t>(exportData.Pos())); // CookedSerialSize
    w.Put<uint32_t>(1); w.Put<uint32_t>(0);            // ObjectName: package pool, index 1
    w.Put<uint64_t>(FPackageObjectIndex::Invalid);     // OuterIndex
    w.Put<uint64_t>(kScriptClassIndex);                // ClassIndex -> the global script object
    w.Put<uint64_t>(FPackageObjectIndex::Invalid);     // SuperIndex
    w.Put<uint64_t>(FPackageObjectIndex::Invalid);     // TemplateIndex
    w.Put<uint64_t>(0x1234);                           // PublicExportHash (UE5 path)
    w.Put<uint32_t>(0);                                // ObjectFlags
    w.Put<uint8_t>(0);                                 // FilterFlags
    w.Zeros(exportStart + 72 - w.Pos());               // pad to the fixed 72-byte record
    CHECK(w.Pos() == exportBundleEntriesOffset);

    // FExportBundleEntry[ExportCount * 2]
    w.Put<uint32_t>(0); w.Put<uint32_t>(0); // export 0, ExportCommandType_Create
    w.Put<uint32_t>(0); w.Put<uint32_t>(1); // export 0, ExportCommandType_Serialize
    CHECK(w.Pos() == graphDataOffset);

    // FExportBundleHeader (UE5: SerialOffset + FirstEntryIndex + EntryCount)
    w.Put<uint64_t>(0);
    w.Put<uint32_t>(0);
    w.Put<uint32_t>(2);
    CHECK(w.Pos() == headerSize);

    w.Raw(exportData.Bytes);
    return w.Bytes;
}

// The same package as a VERSE_CELLS-era (UE5.6) Zen package. Two things differ from the UE5.0 shape above,
// and they are what this fixture exists to pin:
//   * the summary carries three dependency-bundle offsets instead of GraphDataOffset (>= UE5.3), and
//   * an 8-byte FZenPackageCellOffsets block sits between the summary and the name batch, read only when
//     Ar.Ver >= VERSE_CELLS. Those 8 bytes are what a version-misdeclared parse desynchronises on: the name
//     batch is then read out of the cell offsets, whose values become an absurd name count, and the parse
//     dies with "Read size is bigger than remaining archive length" far from the actual cause. Real UE5.6
//     packages store both cell offsets equal to ExportBundleEntriesOffset when a package has no Verse
//     cells — exactly what the pre-VERSE_CELLS branch synthesises — so this fixture does the same.
// From UE5.3 on no export-bundle headers are read; an export's data sits at HeaderSize + CookedSerialOffset.
static std::vector<uint8_t> MakeZenPackageVerseCells()
{
    Writer names;
    names.NameBatch({kPackageName, kExportName});

    constexpr int summarySize = 52 + 8;  // 13 uint32 fields (UE5.3+ layout) + FZenPackageCellOffsets
    constexpr int bulkDataBlock = 16;    // the UE5.4+ pad uint64 (0) + the bulk-data map size int64 (0)
    const int importedHashesOffset = summarySize + static_cast<int>(names.Pos()) + bulkDataBlock;
    const int importMapOffset = importedHashesOffset;              // no imported public export hashes
    const int exportMapOffset = importMapOffset;                   // no imports
    const int exportBundleEntriesOffset = exportMapOffset + 72;    // one FExportMapEntry
    // Nothing on this path reads the dependency bundles, so both offsets just close out the header.
    const int dependencyBundleHeadersOffset = exportBundleEntriesOffset + 2 * 8;
    const int dependencyBundleEntriesOffset = dependencyBundleHeadersOffset;
    const int headerSize = dependencyBundleEntriesOffset;

    Writer exportData;
    exportData.Put<uint16_t>(Fragment(0, 3, true, true));
    exportData.Put<uint8_t>(0x04); // ZeroProp (schema index 2) is stored as zero
    exportData.Put<int32_t>(42);   // IntProp
    exportData.Put<float>(1.5f);   // FloatProp

    Writer w;
    // FZenPackageSummary (UE5.3+)
    w.Put<uint32_t>(0);          // bHasVersioningInfo
    w.Put<uint32_t>(headerSize);
    w.Put<uint32_t>(0);          // Name: package name pool, index 0
    w.Put<uint32_t>(0);          // Name.ExtraIndex
    w.Put<uint32_t>(0x00002000); // PackageFlags: PKG_UnversionedProperties
    w.Put<uint32_t>(headerSize); // CookedHeaderSize
    w.Put<int32_t>(importedHashesOffset);
    w.Put<int32_t>(importMapOffset);
    w.Put<int32_t>(exportMapOffset);
    w.Put<int32_t>(exportBundleEntriesOffset);
    w.Put<int32_t>(dependencyBundleHeadersOffset);
    w.Put<int32_t>(dependencyBundleEntriesOffset);
    w.Put<int32_t>(headerSize);  // ImportedPackageNamesOffset
    CHECK(w.Pos() == 52);

    // FZenPackageCellOffsets — no Verse cells, so both point at the export-bundle entries.
    w.Put<int32_t>(exportBundleEntriesOffset);
    w.Put<int32_t>(exportBundleEntriesOffset);
    CHECK(w.Pos() == summarySize);

    w.Raw(names.Bytes);

    // The bulk-data map block (Ver >= DATA_RESOURCES, and the extra pad from UE5.4 on).
    w.Put<uint64_t>(0); // pad length
    w.Put<int64_t>(0);  // bulk-data map size
    CHECK(w.Pos() == exportMapOffset);

    // FExportMapEntry (72 bytes)
    const int64_t exportStart = w.Pos();
    w.Put<uint64_t>(0);                                       // CookedSerialOffset
    w.Put<uint64_t>(static_cast<uint64_t>(exportData.Pos())); // CookedSerialSize
    w.Put<uint32_t>(1); w.Put<uint32_t>(0);                   // ObjectName: package pool, index 1
    w.Put<uint64_t>(FPackageObjectIndex::Invalid);            // OuterIndex
    w.Put<uint64_t>(kScriptClassIndex);                       // ClassIndex -> the global script object
    w.Put<uint64_t>(FPackageObjectIndex::Invalid);            // SuperIndex
    w.Put<uint64_t>(FPackageObjectIndex::Invalid);            // TemplateIndex
    w.Put<uint64_t>(0x1234);                                  // PublicExportHash
    w.Put<uint32_t>(0);                                       // ObjectFlags
    w.Put<uint8_t>(0);                                        // FilterFlags
    w.Zeros(exportStart + 72 - w.Pos());
    CHECK(w.Pos() == exportBundleEntriesOffset);

    // FExportBundleEntry[ExportCount * 2], read at CellImportMapOffset
    w.Put<uint32_t>(0); w.Put<uint32_t>(0); // export 0, ExportCommandType_Create
    w.Put<uint32_t>(0); w.Put<uint32_t>(1); // export 0, ExportCommandType_Serialize
    CHECK(w.Pos() == headerSize);

    w.Raw(exportData.Bytes);
    return w.Bytes;
}

class TestProvider : public AbstractVfsFileProvider
{
public:
    explicit TestProvider(CUE4Parse::UE4::Versions::EGame game = CUE4Parse::UE4::Versions::GAME_UE5_0)
        : AbstractVfsFileProvider(VersionContainer(game), StringComparer::OrdinalIgnoreCase()) {}
    void Initialize() override {}
};

static void MountFixture(TestProvider& provider, bool verseCells = false)
{
    const auto packageId = FPackageId::FromName(kPackageName);

    const auto global = BuildContainer(0, {{FIoChunkId(0, 0, static_cast<uint8_t>(EIoChunkType5::ScriptObjects)),
                                            MakeScriptObjectsChunk()}}, "");
    provider.RegisterVfs(Archive("global.utoc", global.Toc),
                         [global](const std::string& path) { return Archive(path, global.Ucas); });

    const auto container = BuildContainer(
        kContainerId,
        {{FIoChunkId(packageId.id, 0, static_cast<uint8_t>(EIoChunkType5::ExportBundleData)),
          verseCells ? MakeZenPackageVerseCells() : MakeZenPackage()},
         {FIoChunkId(kContainerId, 0, static_cast<uint8_t>(EIoChunkType5::ContainerHeader)),
          MakeContainerHeaderChunk(packageId)}},
        "TestPackage.uasset");
    provider.RegisterVfs(Archive("pakchunk0.utoc", container.Toc),
                         [container](const std::string& path) { return Archive(path, container.Ucas); });

    provider.Mount();
}

static void TestGlobalDataAndContainerHeader()
{
    TestProvider provider;
    MountFixture(provider);

    // global.utoc has no directory index, so it never mounts — but it still yields IoGlobalData.
    CHECK(provider.MountedVfs().size() == 1);
    CHECK(provider.GlobalData() != nullptr);
    if (provider.GlobalData() != nullptr)
    {
        CHECK(provider.GlobalData()->GlobalNameMap.size() == 1);
        CHECK(provider.GlobalData()->GlobalNameMap[0].ToString() == kClassName);
        CHECK(provider.GlobalData()->ScriptObjectEntriesMap.size() == 1);
        CHECK(provider.GlobalData()->ScriptObjectEntriesMap.count(FPackageObjectIndex(kScriptClassIndex)) == 1);
    }

    // The container header is read lazily off its chunk and its store entry is findable by package id.
    const auto packageId = FPackageId::FromName(kPackageName);
    const auto* storeEntry = provider.TryFindStoreEntry(packageId);
    CHECK(storeEntry != nullptr);
    if (storeEntry != nullptr)
    {
        CHECK(storeEntry->ExportCount == 1);
        CHECK(storeEntry->ExportBundleCount == 1);
        CHECK(storeEntry->ImportedPackages.empty());
    }
    CHECK(provider.TryFindStoreEntry(FPackageId(0xDEAD)) == nullptr);

    // The package is indexed both by path and by package id.
    CHECK(provider.TryGetGameFile("Game/TestPackage.uasset") != nullptr);
    CHECK(provider.FilesById().count(packageId) == 1);
}

static void TestIoPackageLoading()
{
    TestProvider provider;
    auto mappings = std::make_shared<MemoryUsmapProvider>();
    mappings->Load(MakeUsmap());
    provider.MappingsContainer = mappings;
    MountFixture(provider);

    auto* package = dynamic_cast<IoPackage*>(provider.TryLoadPackage("Game/TestPackage.uasset"));
    CHECK(package != nullptr);
    if (package == nullptr) return;

    CHECK(package->GetName() == kPackageName);
    CHECK(package->IsFullyLoaded);
    CHECK(package->NameMap().size() == 2);
    CHECK(package->ImportMap.empty());
    CHECK(package->ExportMap.size() == 1);
    CHECK(package->Summary.ExportCount == 1);
    CHECK(package->Summary.ImportCount == 0);
    CHECK(package->HasFlags(CUE4Parse::UE4::Objects::UObject::PKG_UnversionedProperties));
    CHECK(package->ExportMap[0].PublicExportHash == 0x1234);
    CHECK(package->GetExportIndex(kExportName) == 0);
    CHECK(package->GetExportIndex("Nope") == -1);

    // The export's class resolves through the global script-object table.
    auto* resolvedClass = package->ResolveObjectIndex(package->ExportMap[0].ClassIndex);
    CHECK(resolvedClass != nullptr);
    if (resolvedClass != nullptr) CHECK(resolvedClass->Name().Text() == kClassName);
    CHECK(package->ResolveObjectIndex(FPackageObjectIndex::InvalidObjectIndex()) == nullptr);

    // ...and the export deserializes its unversioned properties through the usmap mappings.
    auto* obj = package->GetExportObject(0);
    CHECK(obj != nullptr);
    if (obj == nullptr) return;

    CHECK(obj->Name == kExportName);
    CHECK(obj->Owner == package);
    CHECK(obj->Outer != nullptr && obj->Outer->Name().Text() == kPackageName); // root export -> the package
    CHECK(obj->Properties.size() == 3);
    if (obj->Properties.size() != 3) return;

    CHECK(obj->Properties[0].Name.Text() == "IntProp");
    auto* ip = dynamic_cast<IntProperty*>(obj->Properties[0].Tag.get());
    CHECK(ip != nullptr && ip->Value == 42);

    CHECK(obj->Properties[1].Name.Text() == "FloatProp");
    auto* fp = dynamic_cast<FloatProperty*>(obj->Properties[1].Tag.get());
    CHECK(fp != nullptr && fp->Value == 1.5f);

    // The zero-masked property is present with a default value and consumed no bytes.
    CHECK(obj->Properties[2].Name.Text() == "ZeroProp");
    auto* zp = dynamic_cast<IntProperty*>(obj->Properties[2].Tag.get());
    CHECK(zp != nullptr && zp->Value == 0);

    // The package is cached: a second load hands back the same object.
    CHECK(provider.TryLoadPackage("Game/TestPackage.uasset") == package);
    CHECK(package->GetExportObject(0) == obj);
    CHECK(package->GetExportObject(1) == nullptr);

    // Loading by package id reaches the same package.
    CHECK(provider.TryLoadPackage(FPackageId::FromName(kPackageName)) == package);
}

static void TestMissingMappingsIsAnError()
{
    // Same fixture, no mappings container: CanDeserialize must refuse rather than misread the export data.
    TestProvider provider;
    MountFixture(provider);

    bool threw = false;
    try { provider.LoadPackage("Game/TestPackage.uasset"); }
    catch (const CUE4Parse::UE4::Exceptions::MappingException&) { threw = true; }
    CHECK(threw);
}

static void TestMissingGlobalDataIsAnError()
{
    // A Zen package without a global container cannot be serialized at all (C#'s ParserException).
    TestProvider provider;
    const auto packageId = FPackageId::FromName(kPackageName);
    const auto container = BuildContainer(
        kContainerId,
        {{FIoChunkId(packageId.id, 0, static_cast<uint8_t>(EIoChunkType5::ExportBundleData)), MakeZenPackage()},
         {FIoChunkId(kContainerId, 0, static_cast<uint8_t>(EIoChunkType5::ContainerHeader)),
          MakeContainerHeaderChunk(packageId)}},
        "TestPackage.uasset");
    provider.RegisterVfs(Archive("pakchunk0.utoc", container.Toc),
                         [container](const std::string& path) { return Archive(path, container.Ucas); });
    provider.Mount();

    CHECK(provider.GlobalData() == nullptr);
    bool threw = false;
    try { provider.LoadPackage("Game/TestPackage.uasset"); }
    catch (const CUE4Parse::UE4::Exceptions::ParserException&) { threw = true; }
    CHECK(threw);
    CHECK(provider.TryLoadPackage("Game/TestPackage.uasset") == nullptr);
}

// The VERSE_CELLS gate, both ways. This is worth pinning because getting it wrong does not look like a
// layout error: the same bytes read at a pre-VERSE_CELLS version desynchronise by 8 and blow up inside the
// name batch with an out-of-range read, which reads as a container/decompression bug rather than a version
// one. A whole real-game run was once misdiagnosed that way — every uncompressed package "failed to
// extract" when in fact the game was UE5.6 and the harness had declared UE5.4.
static void TestVerseCellsGate()
{
    // Read at UE5.6, the 8-byte cell-offsets block is consumed and everything past it lines up.
    {
        TestProvider provider(CUE4Parse::UE4::Versions::GAME_UE5_6);
        auto mappings = std::make_shared<MemoryUsmapProvider>();
        mappings->Load(MakeUsmap());
        provider.MappingsContainer = mappings;
        MountFixture(provider, /*verseCells=*/true);

        auto* package = dynamic_cast<IoPackage*>(provider.TryLoadPackage("Game/TestPackage.uasset"));
        CHECK(package != nullptr);
        if (package == nullptr) return;

        CHECK(package->GetName() == kPackageName);
        CHECK(package->NameMap().size() == 2);
        CHECK(package->ExportMap.size() == 1);
        CHECK(package->ExportMap[0].PublicExportHash == 0x1234);
        CHECK(package->BulkDataMap.empty());

        // From UE5.3 on there are no export-bundle headers: the export's data is located by
        // HeaderSize + CookedSerialOffset instead, so reaching its values proves that path too.
        auto* obj = package->GetExportObject(0);
        CHECK(obj != nullptr);
        if (obj == nullptr) return;
        CHECK(obj->Properties.size() == 3);
        if (obj->Properties.size() != 3) return;
        auto* ip = dynamic_cast<IntProperty*>(obj->Properties[0].Tag.get());
        CHECK(ip != nullptr && ip->Value == 42);
        auto* fp = dynamic_cast<FloatProperty*>(obj->Properties[1].Tag.get());
        CHECK(fp != nullptr && fp->Value == 1.5f);
    }

    // The same bytes at UE5.5 (Ver 1013 < VERSE_CELLS 1015): the cell offsets are not consumed, so the name
    // batch is read out of them and the count is nonsense. This must fail loudly, not read garbage.
    {
        TestProvider provider(CUE4Parse::UE4::Versions::GAME_UE5_5);
        auto mappings = std::make_shared<MemoryUsmapProvider>();
        mappings->Load(MakeUsmap());
        provider.MappingsContainer = mappings;
        MountFixture(provider, /*verseCells=*/true);

        bool threw = false;
        try { provider.LoadPackage("Game/TestPackage.uasset"); }
        catch (const std::exception&) { threw = true; }
        CHECK(threw);
    }
}

int main()
{
    TestUsmapParsing();
    TestUnversionedHeader();
    TestGlobalDataAndContainerHeader();
    TestIoPackageLoading();
    TestMissingMappingsIsAnError();
    TestMissingGlobalDataIsAnError();
    TestVerseCellsGate();

    if (g_failures == 0) std::printf("test_zen_package: all checks passed\n");
    else std::printf("test_zen_package: %d check(s) failed\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
