// End-to-end test for lazy export object loading: hand-build a whole .uasset (summary + name/import/export
// maps + one export's serialized tagged properties), read it with Package, then load the export via
// GetExportObject / ResolvedObject::Load and verify its properties, outer/class chain, flags and caching.
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "UE4/Assets/Package.h"
#include "UE4/Assets/ResolvedObject.h"
#include "UE4/Assets/Exports/UObject.h"
#include "UE4/Assets/Exports/EObjectFlags.h"
#include "UE4/Assets/Objects/FPropertyTag.h"
#include "UE4/Assets/Objects/Properties/IntProperty.h"
#include "UE4/Objects/UObject/FPackageFileSummary.h"
#include "UE4/Objects/UObject/ObjectResource.h"
#include "UE4/Readers/FByteArchive.h"
#include "UE4/Versions/VersionContainer.h"
#include "UE4/Versions/ObjectVersion.h"

using namespace CUE4Parse::UE4::Assets;
using namespace CUE4Parse::UE4::Assets::Exports;
using namespace CUE4Parse::UE4::Assets::Objects::Properties;
using namespace CUE4Parse::UE4::Objects::UObject;
using namespace CUE4Parse::UE4::Readers;
using namespace CUE4Parse::UE4::Versions;

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
    const std::vector<std::string> pool =
        {"None", "Core", "Class", "Engine", "MyImport", "MyExport", "MyPackage", "IntProperty", "Health"};
    enum : int32_t { N_None = 0, N_Core, N_Class, N_Engine, N_MyImport, N_MyExport, N_MyPackage, N_IntProperty, N_Health };
    const int32_t nameCount = static_cast<int32_t>(pool.size());
    const int32_t importCount = 2;
    const int32_t exportCount = 1;

    // --- name section: each entry is an FString + 4 hash bytes (NAME_HASHES_SERIALIZED path). ---
    auto buildNames = [&]() {
        std::vector<uint8_t> b;
        for (const auto& s : pool) { AppendFString(b, s); AppendLE<uint32_t>(b, 0); }
        return b;
    };

    // --- import section (2 entries): MyImport (outer -> Engine), Engine (outermost). ---
    auto buildImports = [&]() {
        std::vector<uint8_t> b;
        AppendFName(b, N_Core); AppendFName(b, N_Class); AppendLE<int32_t>(b, -2); AppendFName(b, N_MyImport); AppendFName(b, N_MyPackage);
        AppendFName(b, N_Core); AppendFName(b, N_Class); AppendLE<int32_t>(b, 0);  AppendFName(b, N_Engine);   AppendFName(b, N_MyPackage);
        return b;
    };

    // --- export section (1 entry), parameterized by its serial range. ---
    auto buildExports = [&](int64_t serialOffset, int64_t serialSize) {
        std::vector<uint8_t> b;
        AppendLE<int32_t>(b, -1);              // ClassIndex   -> import[0] "MyImport"
        AppendLE<int32_t>(b, 0);               // SuperIndex   = null
        AppendLE<int32_t>(b, 0);               // TemplateIndex
        AppendLE<int32_t>(b, 0);               // OuterIndex   = null (package-level export)
        AppendFName(b, N_MyExport);            // ObjectName
        AppendLE<uint32_t>(b, static_cast<uint32_t>(RF_Public)); // ObjectFlags (not RF_ClassDefaultObject)
        AppendLE<int64_t>(b, serialSize);      // SerialSize
        AppendLE<int64_t>(b, serialOffset);    // SerialOffset
        AppendLE<int32_t>(b, 0);               // ForcedExport
        AppendLE<int32_t>(b, 0);               // NotForClient
        AppendLE<int32_t>(b, 0);               // NotForServer
        AppendLE<uint32_t>(b, 0); AppendLE<uint32_t>(b, 0); AppendLE<uint32_t>(b, 0); AppendLE<uint32_t>(b, 0); // PackageGuid
        AppendLE<uint32_t>(b, 0);              // PackageFlags
        AppendLE<int32_t>(b, 1);               // NotAlwaysLoadedForEditorGame
        AppendLE<int32_t>(b, 1);               // IsAsset
        AppendLE<int32_t>(b, -1);              // FirstExportDependency
        AppendLE<int32_t>(b, 0);               // SerializationBeforeSerializationDependencies
        AppendLE<int32_t>(b, 0);               // CreateBeforeSerializationDependencies
        AppendLE<int32_t>(b, 0);               // SerializationBeforeCreateDependencies
        AppendLE<int32_t>(b, 0);               // CreateBeforeCreateDependencies
        return b;
    };

    // --- the export's serialized data: one tagged IntProperty "Health" = 99, then the None terminator and
    //     the trailing object-guid bool (hasGuid = false). ---
    auto buildExportData = [&]() {
        std::vector<uint8_t> b;
        AppendFName(b, N_Health);              // tag Name
        AppendFName(b, N_IntProperty);         // tag PropertyType
        AppendLE<int32_t>(b, 4);               // Size (value bytes)
        AppendLE<int32_t>(b, 0);               // ArrayIndex
        b.push_back(0);                        // HasPropertyGuid flag (ReadFlag = 1 byte)
        AppendLE<int32_t>(b, 99);              // value
        AppendFName(b, N_None);                // terminating "None" tag
        AppendLE<int32_t>(b, 0);               // ObjectGuid present? (ReadBoolean = int32) -> false
        return b;
    };

    // --- summary (mirrors the known-good FPackageFileSummary layout). ---
    auto buildSummary = [&](int32_t nameOffset, int32_t importOffset, int32_t exportOffset) {
        std::vector<uint8_t> b;
        AppendLE<uint32_t>(b, FPackageFileSummary::PACKAGE_FILE_TAG);
        AppendLE<int32_t>(b, -7);          // legacyFileVersion
        AppendLE<int32_t>(b, 864);         // FileVersionUE3
        AppendLE<int32_t>(b, UE4_AUTO);    // FileVersionUE4
        AppendLE<int32_t>(b, 0);           // FileVersionLicenseeUE
        AppendLE<int32_t>(b, 0);           // CustomVersion count
        AppendLE<int32_t>(b, 4096);        // TotalHeaderSize
        AppendFString(b, "MyPackage");     // PackageName
        AppendLE<uint32_t>(b, 0);          // PackageFlags (PKG_None)
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
        AppendLE<uint32_t>(b, 0x11111111u); AppendLE<uint32_t>(b, 0x22222222u); AppendLE<uint32_t>(b, 0x33333333u); AppendLE<uint32_t>(b, 0x44444444u); // Guid
        AppendLE<uint32_t>(b, 0xAAAAAAAAu); AppendLE<uint32_t>(b, 0xBBBBBBBBu); AppendLE<uint32_t>(b, 0xCCCCCCCCu); AppendLE<uint32_t>(b, 0xDDDDDDDDu); // PersistentGuid
        AppendLE<int32_t>(b, 1);           // Generations count
        AppendLE<int32_t>(b, exportCount); AppendLE<int32_t>(b, nameCount);
        AppendLE<uint16_t>(b, 4); AppendLE<uint16_t>(b, 27); AppendLE<uint16_t>(b, 2); AppendLE<uint32_t>(b, 12345); AppendFString(b, "++UE4+Release-4.27");
        AppendLE<uint16_t>(b, 4); AppendLE<uint16_t>(b, 26); AppendLE<uint16_t>(b, 1); AppendLE<uint32_t>(b, 6789);  AppendFString(b, "++UE4+Release-4.26");
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

    // Summary length is offset-value-independent; export section length is serial-range-value-independent.
    const int32_t summaryLen = static_cast<int32_t>(buildSummary(0, 0, 0).size());
    const std::vector<uint8_t> names = buildNames();
    const std::vector<uint8_t> imports = buildImports();
    const std::vector<uint8_t> exportData = buildExportData();

    const int32_t nameOffset = summaryLen;
    const int32_t importOffset = nameOffset + static_cast<int32_t>(names.size());
    const int32_t exportOffset = importOffset + static_cast<int32_t>(imports.size());
    const int32_t exportSectionLen = static_cast<int32_t>(buildExports(0, 0).size());
    const int32_t serialOffset = exportOffset + exportSectionLen;
    const auto serialSize = static_cast<int64_t>(exportData.size());

    const std::vector<uint8_t> exports = buildExports(serialOffset, serialSize);

    std::vector<uint8_t> buf = buildSummary(nameOffset, importOffset, exportOffset);
    buf.insert(buf.end(), names.begin(), names.end());
    buf.insert(buf.end(), imports.begin(), imports.end());
    buf.insert(buf.end(), exports.begin(), exports.end());
    buf.insert(buf.end(), exportData.begin(), exportData.end());

    FByteArchive uasset("MyPackage.uasset", buf, VC);
    Package pkg(uasset);

    CHECK(pkg.ExportMap.size() == 1);
    CHECK(pkg.ExportsLazy.size() == 1);
    CHECK(pkg.ExportMap[0].SerialOffset == serialOffset);
    CHECK(pkg.ExportMap[0].SerialSize == serialSize);

    // --- load the export lazily ---
    UObject* obj = pkg.GetExportObject(0);
    CHECK(obj != nullptr);
    if (obj)
    {
        CHECK(obj->Name == "MyExport");

        // Its one tagged property.
        CHECK(obj->Properties.size() == 1);
        if (obj->Properties.size() == 1)
        {
            CHECK(obj->Properties[0].Name.Text() == "Health");
            auto* intProp = dynamic_cast<IntProperty*>(obj->Properties[0].Tag.get());
            CHECK(intProp != nullptr);
            if (intProp) CHECK(intProp->Value == 99);
        }

        // No trailing ObjectGuid (hasGuid was false).
        CHECK(!obj->ObjectGuid.has_value());

        // Outer falls back to the package itself; Class resolves ClassIndex(-1) -> "MyImport".
        CHECK(obj->Outer != nullptr && obj->Outer->Name().Text() == "MyPackage");
        CHECK(obj->Class != nullptr && obj->Class->Name().Text() == "MyImport");

        // Load flags applied.
        CHECK((obj->Flags & RF_WasLoaded) != 0);
        CHECK((obj->Flags & RF_LoadCompleted) != 0);
    }

    // --- caching: second call returns the same instance ---
    CHECK(pkg.GetExportObject(0) == obj);

    // --- out-of-range indices return null ---
    CHECK(pkg.GetExportObject(1) == nullptr);
    CHECK(pkg.GetExportObject(-1) == nullptr);

    // --- reach the same object through a resolved export index ---
    {
        FPackageIndex expIdx(&pkg, 1); // export[0]
        ResolvedObject* resolved = pkg.ResolvePackageIndex(&expIdx);
        CHECK(resolved != nullptr);
        if (resolved)
        {
            CHECK(resolved->Object() == obj);
            CHECK(resolved->Load<UObject>() == obj);
        }
    }

    if (g_failures == 0)
    {
        std::cout << "All package-loading tests passed.\n";
        return 0;
    }
    std::cout << g_failures << " check(s) failed.\n";
    return 1;
}
