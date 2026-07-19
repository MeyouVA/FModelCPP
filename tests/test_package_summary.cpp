// Round-trip test for FPackageFileSummary: hand-build a modern UE4 package header and verify parse.
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "UE4/Objects/UObject/FPackageFileSummary.h"
#include "UE4/Readers/FByteArchive.h"
#include "UE4/Versions/ObjectVersion.h"

using namespace CUE4Parse::UE4::Objects::UObject;
using namespace CUE4Parse::UE4::Readers;
using namespace CUE4Parse::UE4::Versions;
using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

static int g_failures = 0;
#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::cerr << "FAIL: " << #cond << " (line " << __LINE__ << ")\n";  \
            ++g_failures;                                                      \
        }                                                                      \
    } while (0)

template <typename T>
static void AppendLE(std::vector<uint8_t>& buf, T value)
{
    uint8_t tmp[sizeof(T)];
    std::memcpy(tmp, &value, sizeof(T));
    buf.insert(buf.end(), tmp, tmp + sizeof(T));
}

// Length-prefixed ANSI FString: int32 length incl. null terminator, then bytes + null.
static void AppendFString(std::vector<uint8_t>& buf, const std::string& s)
{
    AppendLE<int32_t>(buf, static_cast<int32_t>(s.size() + 1));
    buf.insert(buf.end(), s.begin(), s.end());
    buf.push_back(0);
}

int main()
{
    // FileVersionUE4 == AUTOMATIC_VERSION: every named UE4 threshold is satisfied, and with UE5 == 0
    // every UE5-gated block is skipped — a deterministic, wide path through the parser.
    const int32_t UE4_AUTO = static_cast<int32_t>(EUnrealEngineObjectUE4Version::AUTOMATIC_VERSION);

    std::vector<uint8_t> buf;
    AppendLE<uint32_t>(buf, FPackageFileSummary::PACKAGE_FILE_TAG); // Tag
    AppendLE<int32_t>(buf, -7);                                     // legacyFileVersion (Optimized custom vers; skips texture-alloc block)
    AppendLE<int32_t>(buf, 864);                                   // FileVersionUE3
    AppendLE<int32_t>(buf, UE4_AUTO);                             // FileVersionUE4
    // (no UE5 read: legacyFileVersion (-7) > -8)
    AppendLE<int32_t>(buf, 0);                                    // FileVersionLicenseeUE (LIC_NONE)
    AppendLE<int32_t>(buf, 0);                                    // CustomVersion count (Optimized, empty)
    AppendLE<int32_t>(buf, 4096);                                // TotalHeaderSize (UE5<PACKAGE_SAVED_HASH branch)
    AppendFString(buf, "MyPackage");                             // PackageName
    AppendLE<uint32_t>(buf, 0);                                  // PackageFlags (PKG_None -> not filter-editor-only)

    // --- afterPackageFlags ---
    AppendLE<int32_t>(buf, 10);   // NameCount
    AppendLE<int32_t>(buf, 100);  // NameOffset
    AppendFString(buf, "loc42");  // LocalizationId
    AppendLE<int32_t>(buf, 1);    // GatherableTextDataCount
    AppendLE<int32_t>(buf, 200);  // GatherableTextDataOffset
    AppendLE<int32_t>(buf, 5);    // ExportCount
    AppendLE<int32_t>(buf, 300);  // ExportOffset
    AppendLE<int32_t>(buf, 3);    // ImportCount
    AppendLE<int32_t>(buf, 400);  // ImportOffset
    AppendLE<int32_t>(buf, 500);  // DependsOffset
    AppendLE<int32_t>(buf, 2);    // SoftPackageReferencesCount
    AppendLE<int32_t>(buf, 600);  // SoftPackageReferencesOffset
    AppendLE<int32_t>(buf, 700);  // SearchableNamesOffset
    AppendLE<int32_t>(buf, 800);  // ThumbnailTableOffset
    // (ImportTypeHierarchies: UE5-gated + not DeltaForce -> no read)
    // Guid (UE5 < PACKAGE_SAVED_HASH)
    AppendLE<uint32_t>(buf, 0x11111111u);
    AppendLE<uint32_t>(buf, 0x22222222u);
    AppendLE<uint32_t>(buf, 0x33333333u);
    AppendLE<uint32_t>(buf, 0x44444444u);
    // PersistentGuid (not filter-editor-only, UE4 >= ADDED_PACKAGE_OWNER)
    AppendLE<uint32_t>(buf, 0xAAAAAAAAu);
    AppendLE<uint32_t>(buf, 0xBBBBBBBBu);
    AppendLE<uint32_t>(buf, 0xCCCCCCCCu);
    AppendLE<uint32_t>(buf, 0xDDDDDDDDu);
    // Generations (count-prefixed FGenerationInfo)
    AppendLE<int32_t>(buf, 1);    // count
    AppendLE<int32_t>(buf, 5);    // [0].ExportCount
    AppendLE<int32_t>(buf, 10);   // [0].NameCount
    // SavedByEngineVersion
    AppendLE<uint16_t>(buf, 4);
    AppendLE<uint16_t>(buf, 27);
    AppendLE<uint16_t>(buf, 2);
    AppendLE<uint32_t>(buf, 12345);
    AppendFString(buf, "++UE4+Release-4.27");
    // CompatibleWithEngineVersion
    AppendLE<uint16_t>(buf, 4);
    AppendLE<uint16_t>(buf, 26);
    AppendLE<uint16_t>(buf, 1);
    AppendLE<uint32_t>(buf, 6789);
    AppendFString(buf, "++UE4+Release-4.26");
    AppendLE<int32_t>(buf, 0);    // CompressionFlags (COMPRESS_None)
    AppendLE<int32_t>(buf, 0);    // compressedChunks count (0)
    AppendLE<int32_t>(buf, 999);  // PackageSource
    AppendLE<int32_t>(buf, 0);    // additionalPackagesToCook count (0)
    // (legacyFileVersion (-7) > -7 is false -> no numTextureAllocations)
    AppendLE<int32_t>(buf, 1234); // AssetRegistryDataOffset
    AppendLE<int64_t>(buf, 900000); // BulkDataStartOffset (serialized as long)
    AppendLE<int32_t>(buf, 111);  // WorldTileInfoDataOffset
    AppendLE<int32_t>(buf, 0);    // ChunkIds count (0)
    AppendLE<int32_t>(buf, 7);    // PreloadDependencyCount
    AppendLE<int32_t>(buf, 222);  // PreloadDependencyOffset
    // (Names/PayloadToc/DataResource are UE5-gated -> defaults)

    FByteArchive ar("pkg", buf);
    FPackageFileSummary sum(ar);

    CHECK(sum.Tag == FPackageFileSummary::PACKAGE_FILE_TAG);
    CHECK(sum.FileVersionUE.FileVersionUE4 == UE4_AUTO);
    CHECK(sum.FileVersionUE.FileVersionUE3 == 864);
    CHECK(!sum.bUnversioned);
    CHECK(static_cast<int32_t>(sum.FileVersionLicenseeUE) == 0);
    CHECK(sum.CustomVersionContainer.Versions.empty());
    CHECK(sum.TotalHeaderSize == 4096);
    CHECK(sum.PackageName == "MyPackage");
    CHECK(sum.NameCount == 10);
    CHECK(sum.NameOffset == 100);
    CHECK(sum.LocalizationId == "loc42");
    CHECK(sum.GatherableTextDataCount == 1);
    CHECK(sum.ExportCount == 5);
    CHECK(sum.ExportOffset == 300);
    CHECK(sum.ImportCount == 3);
    CHECK(sum.ImportOffset == 400);
    CHECK(sum.DependsOffset == 500);
    CHECK(sum.SoftPackageReferencesCount == 2);
    CHECK(sum.SearchableNamesOffset == 700);
    CHECK(sum.ThumbnailTableOffset == 800);
    CHECK(sum.Guid == FGuid(0x11111111u, 0x22222222u, 0x33333333u, 0x44444444u));
    CHECK(sum.PersistentGuid == FGuid(0xAAAAAAAAu, 0xBBBBBBBBu, 0xCCCCCCCCu, 0xDDDDDDDDu));
    CHECK(sum.Generations.size() == 1);
    CHECK(sum.Generations[0].ExportCount == 5 && sum.Generations[0].NameCount == 10);
    CHECK(sum.SavedByEngineVersion.has_value());
    CHECK(sum.SavedByEngineVersion->Major == 4 && sum.SavedByEngineVersion->Minor == 27 && sum.SavedByEngineVersion->Patch == 2);
    CHECK(sum.SavedByEngineVersion->Changelist() == 12345);
    CHECK(sum.SavedByEngineVersion->Branch() == "//UE4/Release-4.27");
    CHECK(sum.CompatibleWithEngineVersion.has_value());
    CHECK(sum.CompatibleWithEngineVersion->Minor == 26 && sum.CompatibleWithEngineVersion->Patch == 1);
    CHECK(sum.CompatibleWithEngineVersion->Changelist() == 6789);
    CHECK(sum.CompressionFlags == 0);
    CHECK(sum.PackageSource == 999);
    CHECK(sum.AssetRegistryDataOffset == 1234);
    CHECK(sum.BulkDataStartOffset == 900000);
    CHECK(sum.WorldTileInfoDataOffset == 111);
    CHECK(sum.ChunkIds.empty());
    CHECK(sum.PreloadDependencyCount == 7);
    CHECK(sum.PreloadDependencyOffset == 222);
    CHECK(sum.NamesReferencedFromExportDataCount == 10); // == NameCount (UE5-gated read skipped)
    CHECK(sum.PayloadTocOffset == -1);
    CHECK(sum.DataResourceOffset == -1);

    // The whole buffer should be consumed exactly.
    CHECK(ar.Position == static_cast<int64_t>(buf.size()));

    // Invalid magic should throw.
    {
        std::vector<uint8_t> bad;
        AppendLE<uint32_t>(bad, 0xDEADBEEFu);
        AppendLE<int32_t>(bad, -7);
        FByteArchive badAr("bad", bad);
        bool threw = false;
        try { FPackageFileSummary s(badAr); } catch (const std::exception&) { threw = true; }
        CHECK(threw);
    }

    if (g_failures == 0)
    {
        std::cout << "All FPackageFileSummary tests passed.\n";
        return 0;
    }
    std::cout << g_failures << " check(s) failed.\n";
    return 1;
}
