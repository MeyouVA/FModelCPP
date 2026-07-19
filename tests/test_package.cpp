// End-to-end test for the classic-package reader: hand-build a whole .uasset (summary + name/import/export
// maps) in memory, read it with Package, and verify the maps parse and that import/export indices resolve
// to names and path names.
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "UE4/Assets/Package.h"
#include "UE4/Assets/ResolvedObject.h"
#include "UE4/Objects/UObject/FPackageFileSummary.h"
#include "UE4/Objects/UObject/ObjectResource.h"
#include "UE4/Assets/Exports/EObjectFlags.h"
#include "UE4/Readers/FByteArchive.h"
#include "UE4/Versions/VersionContainer.h"
#include "UE4/Versions/ObjectVersion.h"

using namespace CUE4Parse::UE4::Assets;
using namespace CUE4Parse::UE4::Objects::UObject;
using namespace CUE4Parse::UE4::Readers;
using namespace CUE4Parse::UE4::Versions;
using namespace CUE4Parse::UE4::Assets::Exports;

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

// An FName *reference* inside an asset archive: (nameIndex, extraIndex), both int32 (UE3 >= SPLIT).
static void AppendFName(std::vector<uint8_t>& buf, int32_t nameIndex, int32_t extraIndex = 0)
{
    AppendLE<int32_t>(buf, nameIndex);
    AppendLE<int32_t>(buf, extraIndex);
}

int main()
{
    const int32_t UE4_AUTO = static_cast<int32_t>(EUnrealEngineObjectUE4Version::AUTOMATIC_VERSION);
    // ue3=864 (past FNAME_CHANGE_NAME_SPLIT), ue4=AUTOMATIC (all UE4 gates on), ue5=0 (all UE5 gates off).
    const VersionContainer VC(GAME_UE4_LATEST, ETexturePlatform::DesktopMobile, FPackageFileVersion(864, UE4_AUTO, 0));

    // Name pool.
    const std::vector<std::string> pool = {"None", "Core", "Class", "Engine", "MyImport", "MyExport", "MyPackage"};
    const int32_t nameCount = static_cast<int32_t>(pool.size());
    const int32_t importCount = 2;
    const int32_t exportCount = 1;

    // --- name section: each entry is an FString + 4 hash bytes (NAME_HASHES_SERIALIZED path). ---
    auto buildNames = [&]() {
        std::vector<uint8_t> b;
        for (const auto& s : pool)
        {
            AppendFString(b, s);
            AppendLE<uint32_t>(b, 0); // name hashes (skipped on read)
        }
        return b;
    };

    // --- import section (2 entries). ---
    auto buildImports = [&]() {
        std::vector<uint8_t> b;
        // Import[0] "MyImport": Class Core.Class, Outer -> import[1], Package "MyPackage".
        AppendFName(b, 1);          // ClassPackage = Core
        AppendFName(b, 2);          // ClassName    = Class
        AppendLE<int32_t>(b, -2);   // OuterIndex   -> import index 1 ("Engine")
        AppendFName(b, 4);          // ObjectName   = MyImport
        AppendFName(b, 6);          // PackageName  = MyPackage
        // Import[1] "Engine": Outer null (outermost).
        AppendFName(b, 1);          // ClassPackage = Core
        AppendFName(b, 2);          // ClassName    = Class
        AppendLE<int32_t>(b, 0);    // OuterIndex   = null
        AppendFName(b, 3);          // ObjectName   = Engine
        AppendFName(b, 6);          // PackageName  = MyPackage
        return b;
    };

    // --- export section (1 entry). ---
    auto buildExports = [&]() {
        std::vector<uint8_t> b;
        AppendLE<int32_t>(b, -1);   // ClassIndex   -> import[0] "MyImport"
        AppendLE<int32_t>(b, 0);    // SuperIndex   = null
        AppendLE<int32_t>(b, 0);    // TemplateIndex (read: TemplateIndex_IN_COOKED_EXPORTS)
        AppendLE<int32_t>(b, 0);    // OuterIndex   = null (package-level export)
        AppendFName(b, 5);          // ObjectName   = MyExport
        AppendLE<uint32_t>(b, static_cast<uint32_t>(RF_Public)); // ObjectFlags
        AppendLE<int64_t>(b, 100);  // SerialSize (64-bit)
        AppendLE<int64_t>(b, 500);  // SerialOffset
        AppendLE<int32_t>(b, 0);    // ForcedExport (bool = int32)
        AppendLE<int32_t>(b, 1);    // NotForClient
        AppendLE<int32_t>(b, 0);    // NotForServer
        AppendLE<uint32_t>(b, 0);   // PackageGuid x4 (read: < REMOVE_OBJECT_EXPORT_PACKAGE_GUID)
        AppendLE<uint32_t>(b, 0);
        AppendLE<uint32_t>(b, 0);
        AppendLE<uint32_t>(b, 0);
        AppendLE<uint32_t>(b, 0);   // PackageFlags
        AppendLE<int32_t>(b, 1);    // NotAlwaysLoadedForEditorGame (LOAD_FOR_EDITOR_GAME)
        AppendLE<int32_t>(b, 1);    // IsAsset (COOKED_ASSETS_IN_EDITOR_SUPPORT)
        AppendLE<int32_t>(b, -1);   // FirstExportDependency (PRELOAD_DEPENDENCIES_IN_COOKED_EXPORTS)
        AppendLE<int32_t>(b, 0);    // SerializationBeforeSerializationDependencies
        AppendLE<int32_t>(b, 0);    // CreateBeforeSerializationDependencies
        AppendLE<int32_t>(b, 0);    // SerializationBeforeCreateDependencies
        AppendLE<int32_t>(b, 0);    // CreateBeforeCreateDependencies
        return b;
    };

    // --- summary (mirrors the known-good FPackageFileSummary layout; offsets/counts point at our maps). ---
    auto buildSummary = [&](int32_t nameOffset, int32_t importOffset, int32_t exportOffset) {
        std::vector<uint8_t> b;
        AppendLE<uint32_t>(b, FPackageFileSummary::PACKAGE_FILE_TAG); // Tag
        AppendLE<int32_t>(b, -7);          // legacyFileVersion
        AppendLE<int32_t>(b, 864);         // FileVersionUE3
        AppendLE<int32_t>(b, UE4_AUTO);    // FileVersionUE4
        AppendLE<int32_t>(b, 0);           // FileVersionLicenseeUE
        AppendLE<int32_t>(b, 0);           // CustomVersion count
        AppendLE<int32_t>(b, 4096);        // TotalHeaderSize
        AppendFString(b, "MyPackage");     // PackageName
        AppendLE<uint32_t>(b, 0);          // PackageFlags (PKG_None -> not filter-editor-only)
        // --- afterPackageFlags ---
        AppendLE<int32_t>(b, nameCount);   // NameCount
        AppendLE<int32_t>(b, nameOffset);  // NameOffset
        AppendFString(b, "loc42");         // LocalizationId
        AppendLE<int32_t>(b, 0);           // GatherableTextDataCount
        AppendLE<int32_t>(b, 0);           // GatherableTextDataOffset
        AppendLE<int32_t>(b, exportCount); // ExportCount
        AppendLE<int32_t>(b, exportOffset);// ExportOffset
        AppendLE<int32_t>(b, importCount); // ImportCount
        AppendLE<int32_t>(b, importOffset);// ImportOffset
        AppendLE<int32_t>(b, 0);           // DependsOffset
        AppendLE<int32_t>(b, 0);           // SoftPackageReferencesCount
        AppendLE<int32_t>(b, 0);           // SoftPackageReferencesOffset
        AppendLE<int32_t>(b, 0);           // SearchableNamesOffset
        AppendLE<int32_t>(b, 0);           // ThumbnailTableOffset
        // Guid
        AppendLE<uint32_t>(b, 0x11111111u);
        AppendLE<uint32_t>(b, 0x22222222u);
        AppendLE<uint32_t>(b, 0x33333333u);
        AppendLE<uint32_t>(b, 0x44444444u);
        // PersistentGuid (not filter-editor-only, UE4 >= ADDED_PACKAGE_OWNER)
        AppendLE<uint32_t>(b, 0xAAAAAAAAu);
        AppendLE<uint32_t>(b, 0xBBBBBBBBu);
        AppendLE<uint32_t>(b, 0xCCCCCCCCu);
        AppendLE<uint32_t>(b, 0xDDDDDDDDu);
        // Generations (count-prefixed)
        AppendLE<int32_t>(b, 1);
        AppendLE<int32_t>(b, exportCount); // [0].ExportCount
        AppendLE<int32_t>(b, nameCount);   // [0].NameCount
        // SavedByEngineVersion
        AppendLE<uint16_t>(b, 4);
        AppendLE<uint16_t>(b, 27);
        AppendLE<uint16_t>(b, 2);
        AppendLE<uint32_t>(b, 12345);
        AppendFString(b, "++UE4+Release-4.27");
        // CompatibleWithEngineVersion
        AppendLE<uint16_t>(b, 4);
        AppendLE<uint16_t>(b, 26);
        AppendLE<uint16_t>(b, 1);
        AppendLE<uint32_t>(b, 6789);
        AppendFString(b, "++UE4+Release-4.26");
        AppendLE<int32_t>(b, 0);           // CompressionFlags
        AppendLE<int32_t>(b, 0);           // compressedChunks count
        AppendLE<int32_t>(b, 999);         // PackageSource
        AppendLE<int32_t>(b, 0);           // additionalPackagesToCook count
        AppendLE<int32_t>(b, 1234);        // AssetRegistryDataOffset
        AppendLE<int64_t>(b, 900000);      // BulkDataStartOffset
        AppendLE<int32_t>(b, 111);         // WorldTileInfoDataOffset
        AppendLE<int32_t>(b, 0);           // ChunkIds count
        AppendLE<int32_t>(b, 0);           // PreloadDependencyCount
        AppendLE<int32_t>(b, 0);           // PreloadDependencyOffset
        return b;
    };

    // Offsets are absolute positions in the uasset. Summary length is independent of the offset values
    // (all fixed-size int32s), so measure it once, then place the sections right after it.
    const int32_t summaryLen = static_cast<int32_t>(buildSummary(0, 0, 0).size());
    const std::vector<uint8_t> names = buildNames();
    const std::vector<uint8_t> imports = buildImports();
    const std::vector<uint8_t> exports = buildExports();

    const int32_t nameOffset = summaryLen;
    const int32_t importOffset = nameOffset + static_cast<int32_t>(names.size());
    const int32_t exportOffset = importOffset + static_cast<int32_t>(imports.size());

    std::vector<uint8_t> buf = buildSummary(nameOffset, importOffset, exportOffset);
    buf.insert(buf.end(), names.begin(), names.end());
    buf.insert(buf.end(), imports.begin(), imports.end());
    buf.insert(buf.end(), exports.begin(), exports.end());

    FByteArchive uasset("MyPackage.uasset", buf, VC);
    Package pkg(uasset);

    // --- summary + map sizes ---
    CHECK(pkg.GetName() == "MyPackage");
    CHECK(pkg.Summary.NameCount == nameCount);
    CHECK(pkg.NameMap().size() == static_cast<size_t>(nameCount));
    CHECK(pkg.ImportMap.size() == 2);
    CHECK(pkg.ExportMap.size() == 1);

    // --- import map contents ---
    CHECK(pkg.ImportMap[0].ObjectName.Text() == "MyImport");
    CHECK(pkg.ImportMap[0].ClassPackage.Text() == "Core");
    CHECK(pkg.ImportMap[0].ClassName.Text() == "Class");
    CHECK(pkg.ImportMap[0].PackageName.Text() == "MyPackage");
    CHECK(pkg.ImportMap[0].OuterIndex.Index == -2);
    CHECK(pkg.ImportMap[1].ObjectName.Text() == "Engine");
    CHECK(pkg.ImportMap[1].OuterIndex.IsNull());

    // --- export map contents ---
    CHECK(pkg.ExportMap[0].ObjectName.Text() == "MyExport");
    CHECK(pkg.ExportMap[0].SerialSize == 100);
    CHECK(pkg.ExportMap[0].SerialOffset == 500);
    CHECK(pkg.ExportMap[0].NotForClient);
    CHECK(pkg.ExportMap[0].IsAsset);
    CHECK(pkg.ExportMap[0].ClassIndex.Index == -1);

    // --- resolution: ClassIndex (-1) resolves to import[0]'s name "MyImport" ---
    CHECK(pkg.ExportMap[0].ClassName == "MyImport");
    CHECK(pkg.ExportMap[0].ToString() == "MyExport (MyImport)");

    // --- ResolvePackageIndex directly ---
    {
        FPackageIndex expIdx(&pkg, 1);   // export[0]
        FPackageIndex impIdx(&pkg, -1);  // import[0]
        ResolvedObject* rExp = pkg.ResolvePackageIndex(&expIdx);
        ResolvedObject* rImp = pkg.ResolvePackageIndex(&impIdx);
        CHECK(rExp != nullptr && rExp->Name().Text() == "MyExport");
        CHECK(rImp != nullptr && rImp->Name().Text() == "MyImport");
        // import[0]'s outer chain: Engine (outermost) . MyImport
        CHECK(rImp->GetPathName() == "Engine.MyImport");
        CHECK(pkg.ResolvePackageIndex(nullptr) == nullptr);
        FPackageIndex nullIdx(&pkg, 0);
        CHECK(pkg.ResolvePackageIndex(&nullIdx) == nullptr);
    }

    // --- FPackageIndex::Name() convenience ---
    CHECK(FPackageIndex(&pkg, -1).Name() == "MyImport");
    CHECK(FPackageIndex(&pkg, 1).Name() == "MyExport");

    // --- GetExportIndex ---
    CHECK(pkg.GetExportIndex("MyExport") == 0);
    CHECK(pkg.GetExportIndex("Nope") == -1);

    if (g_failures == 0)
    {
        std::cout << "All package-reader tests passed.\n";
        return 0;
    }
    std::cout << g_failures << " check(s) failed.\n";
    return 1;
}
