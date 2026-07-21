// End-to-end test for UDataTable: a hand-built .uasset with an export of class "DataTable" that carries a
// "RowStruct" ObjectProperty (-> import "MyRowStruct") followed by a 2-row RowMap. Verifies the registry
// builds the concrete UDataTable, RowStructName is recovered from the tagged property, and each row's
// FStructFallback parses its tagged IntProperty cell.
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "FileProvider/IFileProvider.h"
#include "FileProvider/InternationalizationDictionary.h"
#include "UE4/Assets/Package.h"
#include "UE4/Assets/Exports/UObject.h"
#include "UE4/Assets/Exports/EObjectFlags.h"
#include "UE4/Assets/Exports/Engine/UDataTable.h"
#include "UE4/Assets/Objects/FPropertyTag.h"
#include "UE4/Assets/Objects/Properties/IntProperty.h"
#include "UE4/Objects/UObject/FPackageFileSummary.h"
#include "UE4/Readers/FByteArchive.h"
#include "UE4/Versions/VersionContainer.h"
#include "UE4/Versions/ObjectVersion.h"

using namespace CUE4Parse::FileProvider;
using namespace CUE4Parse::UE4::Assets;
using namespace CUE4Parse::UE4::Assets::Exports;
using namespace CUE4Parse::UE4::Assets::Exports::Engine;
using namespace CUE4Parse::UE4::Assets::Objects::Properties;
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

static void AppendFString(std::vector<uint8_t>& buf, const std::string& s)
{
    AppendLE<int32_t>(buf, static_cast<int32_t>(s.size() + 1));
    buf.insert(buf.end(), s.begin(), s.end());
    buf.push_back(0);
}

static void AppendFName(std::vector<uint8_t>& buf, int32_t nameIndex, int32_t extraIndex = 0)
{
    AppendLE<int32_t>(buf, nameIndex);
    AppendLE<int32_t>(buf, extraIndex);
}

static void AppendImport(std::vector<uint8_t>& buf, int32_t classPackage, int32_t className,
                         int32_t outerIndex, int32_t objectName, int32_t packageName)
{
    AppendFName(buf, classPackage);
    AppendFName(buf, className);
    AppendLE<int32_t>(buf, outerIndex);
    AppendFName(buf, objectName);
    AppendFName(buf, packageName);
}

static void AppendExport(std::vector<uint8_t>& buf, int32_t classIndex, int32_t objectName,
                         int64_t serialOffset, int64_t serialSize)
{
    AppendLE<int32_t>(buf, classIndex);  // ClassIndex
    AppendLE<int32_t>(buf, 0);           // SuperIndex
    AppendLE<int32_t>(buf, 0);           // TemplateIndex
    AppendLE<int32_t>(buf, 0);           // OuterIndex (package-level)
    AppendFName(buf, objectName);        // ObjectName
    AppendLE<uint32_t>(buf, static_cast<uint32_t>(RF_Public)); // ObjectFlags
    AppendLE<int64_t>(buf, serialSize);
    AppendLE<int64_t>(buf, serialOffset);
    AppendLE<int32_t>(buf, 0);           // ForcedExport
    AppendLE<int32_t>(buf, 0);           // NotForClient
    AppendLE<int32_t>(buf, 0);           // NotForServer
    for (int i = 0; i < 4; i++) AppendLE<uint32_t>(buf, 0); // PackageGuid
    AppendLE<uint32_t>(buf, 0);          // PackageFlags
    AppendLE<int32_t>(buf, 1);           // NotAlwaysLoadedForEditorGame
    AppendLE<int32_t>(buf, 1);           // IsAsset
    AppendLE<int32_t>(buf, -1);          // FirstExportDependency
    AppendLE<int32_t>(buf, 0); AppendLE<int32_t>(buf, 0); AppendLE<int32_t>(buf, 0); AppendLE<int32_t>(buf, 0);
}

static std::vector<uint8_t> BuildSummary(int32_t nameCount, int32_t nameOffset,
                                         int32_t importCount, int32_t importOffset,
                                         int32_t exportCount, int32_t exportOffset,
                                         const std::string& packageName)
{
    const int32_t UE4_AUTO = static_cast<int32_t>(EUnrealEngineObjectUE4Version::AUTOMATIC_VERSION);
    std::vector<uint8_t> b;
    AppendLE<uint32_t>(b, FPackageFileSummary::PACKAGE_FILE_TAG);
    AppendLE<int32_t>(b, -7);
    AppendLE<int32_t>(b, 864);
    AppendLE<int32_t>(b, UE4_AUTO);
    AppendLE<int32_t>(b, 0);
    AppendLE<int32_t>(b, 0);           // CustomVersion count
    AppendLE<int32_t>(b, 4096);        // TotalHeaderSize
    AppendFString(b, packageName);
    AppendLE<uint32_t>(b, 0);          // PackageFlags
    AppendLE<int32_t>(b, nameCount);
    AppendLE<int32_t>(b, nameOffset);
    AppendFString(b, "loc42");         // LocalizationId
    AppendLE<int32_t>(b, 0); AppendLE<int32_t>(b, 0); // GatherableTextData
    AppendLE<int32_t>(b, exportCount);
    AppendLE<int32_t>(b, exportOffset);
    AppendLE<int32_t>(b, importCount);
    AppendLE<int32_t>(b, importOffset);
    AppendLE<int32_t>(b, 0);           // DependsOffset
    AppendLE<int32_t>(b, 0); AppendLE<int32_t>(b, 0); // SoftPackageReferences
    AppendLE<int32_t>(b, 0);           // SearchableNamesOffset
    AppendLE<int32_t>(b, 0);           // ThumbnailTableOffset
    for (int i = 0; i < 8; i++) AppendLE<uint32_t>(b, 0x11111111u); // Guid + PersistentGuid
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
    AppendLE<int32_t>(b, 0); AppendLE<int32_t>(b, 0); // PreloadDependency count/offset
    return b;
}

struct PackageBuilder
{
    std::vector<std::string> Pool;
    std::vector<uint8_t> Imports;
    std::vector<uint8_t> ExportData;
    std::string PackageName;
    int32_t ImportCount = 0;

    template <typename F>
    std::vector<uint8_t> Assemble(int32_t exportCount, F buildExports) const
    {
        std::vector<uint8_t> names;
        for (const auto& s : Pool) { AppendFString(names, s); AppendLE<uint32_t>(names, 0); }

        const auto nameCount = static_cast<int32_t>(Pool.size());
        const auto summaryLen = static_cast<int32_t>(
            BuildSummary(nameCount, 0, ImportCount, 0, exportCount, 0, PackageName).size());
        const int32_t nameOffset = summaryLen;
        const auto importOffset = nameOffset + static_cast<int32_t>(names.size());
        const auto exportOffset = importOffset + static_cast<int32_t>(Imports.size());
        const auto exportSectionLen = static_cast<int32_t>(buildExports(0).size());
        const int32_t serialOffset = exportOffset + exportSectionLen;

        std::vector<uint8_t> buf = BuildSummary(nameCount, nameOffset, ImportCount, importOffset,
                                                exportCount, exportOffset, PackageName);
        buf.insert(buf.end(), names.begin(), names.end());
        buf.insert(buf.end(), Imports.begin(), Imports.end());
        const std::vector<uint8_t> exports = buildExports(serialOffset);
        buf.insert(buf.end(), exports.begin(), exports.end());
        buf.insert(buf.end(), ExportData.begin(), ExportData.end());
        return buf;
    }
};

class TestFileProvider : public IFileProvider
{
public:
    VersionContainer Versions;
    InternationalizationDictionary Internationalization;

    explicit TestFileProvider(VersionContainer versions) : Versions(std::move(versions)) {}

    void AddPackage(const std::string& path, std::string archiveName, std::vector<uint8_t> data)
    {
        _files.emplace(path, std::make_unique<FByteArchive>(std::move(archiveName), std::move(data), Versions));
    }

    const VersionContainer& GetVersions() const override { return Versions; }
    InternationalizationDictionary& GetInternationalization() override { return Internationalization; }

    IPackage* TryLoadPackage(const std::string& path) override
    {
        const auto loaded = _loaded.find(path);
        if (loaded != _loaded.end()) return loaded->second.get();
        const auto file = _files.find(path);
        if (file == _files.end()) return nullptr;
        auto pkg = std::make_unique<Package>(*file->second, nullptr, this);
        auto* raw = pkg.get();
        _loaded.emplace(path, std::move(pkg));
        return raw;
    }

private:
    std::map<std::string, std::unique_ptr<FByteArchive>> _files;
    std::map<std::string, std::unique_ptr<Package>> _loaded;
};

// Appends a classic tagged IntProperty (no TagData, no property guid): Name, Type, Size, ArrayIndex, guid flag, value.
static void AppendIntProperty(std::vector<uint8_t>& buf, int32_t nameIdx, int32_t typeIdx, int32_t value)
{
    AppendFName(buf, nameIdx);      // Name
    AppendFName(buf, typeIdx);      // Type = "IntProperty"
    AppendLE<int32_t>(buf, 4);      // Size
    AppendLE<int32_t>(buf, 0);      // ArrayIndex
    buf.push_back(0);               // HasPropertyGuid = false
    AppendLE<int32_t>(buf, value);  // value
}

int main()
{
    const int32_t UE4_AUTO = static_cast<int32_t>(EUnrealEngineObjectUE4Version::AUTOMATIC_VERSION);
    const VersionContainer VC(GAME_UE4_LATEST, ETexturePlatform::DesktopMobile, FPackageFileVersion(864, UE4_AUTO, 0));

    TestFileProvider provider(VC);

    // ---------- DataTablePkg: export "MyDataTable" of class "DataTable" with a RowStruct + 2 rows. ----------
    {
        PackageBuilder b;
        b.Pool = {"None", "Core", "Class", "Package", "ScriptStruct", "DataTable", "/Script/Engine",
                  "MyDataTable", "MyRowStruct", "RowStruct", "ObjectProperty", "Value", "IntProperty",
                  "Row0", "Row1"};
        enum : int32_t {
            D_None = 0, D_Core, D_Class, D_Package, D_ScriptStruct, D_DataTable, D_ScriptEngine,
            D_MyDataTable, D_MyRowStruct, D_RowStruct, D_ObjectProperty, D_Value, D_IntProperty, D_Row0, D_Row1
        };
        b.PackageName = "DataTablePkg";
        b.ImportCount = 3;

        AppendImport(b.Imports, D_Core, D_Class,        -2, D_DataTable,   D_None); // [0] class DataTable, outer -> import[1]
        AppendImport(b.Imports, D_Core, D_Package,       0, D_ScriptEngine, D_None); // [1] /Script/Engine
        AppendImport(b.Imports, D_Core, D_ScriptStruct, -2, D_MyRowStruct, D_None); // [2] the row struct, outer -> import[1]

        // Export data: one tagged "RowStruct" ObjectProperty (-> import[2]), None terminator, no guid,
        // then the DataTable body (numRows + per-row FName + FStructFallback).
        AppendFName(b.ExportData, D_RowStruct);       // property Name
        AppendFName(b.ExportData, D_ObjectProperty);  // property Type
        AppendLE<int32_t>(b.ExportData, 4);           // Size
        AppendLE<int32_t>(b.ExportData, 0);           // ArrayIndex
        b.ExportData.push_back(0);                    // HasPropertyGuid = false
        AppendLE<int32_t>(b.ExportData, -3);          // ObjectProperty value = FPackageIndex -> import[2]
        AppendFName(b.ExportData, D_None);            // properties terminator
        AppendLE<int32_t>(b.ExportData, 0);           // hasObjectGuid = false

        AppendLE<int32_t>(b.ExportData, 2);           // numRows
        AppendFName(b.ExportData, D_Row0);            // row 0 name
        AppendIntProperty(b.ExportData, D_Value, D_IntProperty, 100);
        AppendFName(b.ExportData, D_None);            //   row 0 FStructFallback terminator
        AppendFName(b.ExportData, D_Row1);            // row 1 name
        AppendIntProperty(b.ExportData, D_Value, D_IntProperty, 200);
        AppendFName(b.ExportData, D_None);            //   row 1 FStructFallback terminator

        const auto serialSize = static_cast<int64_t>(b.ExportData.size());
        auto buf = b.Assemble(1, [&](int32_t serialOffset) {
            std::vector<uint8_t> e;
            AppendExport(e, -1, D_MyDataTable, serialOffset, serialSize); // ClassIndex -> import[0] DataTable
            return e;
        });
        provider.AddPackage("/Game/DataTablePkg", "DataTablePkg.uasset", std::move(buf));
    }

    // ---------- the DataTable export builds as a UDataTable and parses its rows ----------
    auto* pkg = dynamic_cast<Package*>(provider.TryLoadPackage("/Game/DataTablePkg"));
    CHECK(pkg != nullptr);
    if (!pkg) { std::cout << g_failures << " check(s) failed.\n"; return 1; }

    UObject* obj = pkg->GetExportObject(0);
    CHECK(obj != nullptr);
    auto* dt = dynamic_cast<UDataTable*>(obj);
    CHECK(dt != nullptr); // ObjectTypeRegistry produced the concrete type
    if (dt)
    {
        CHECK(dt->Name == "MyDataTable");
        CHECK(dt->RowStructName.has_value());
        if (dt->RowStructName.has_value()) CHECK(*dt->RowStructName == "MyRowStruct");

        CHECK(dt->RowMap.size() == 2);
        if (dt->RowMap.size() == 2)
        {
            CHECK(dt->RowMap[0].first.Text() == "Row0");
            CHECK(dt->RowMap[1].first.Text() == "Row1");

            const int32_t expected[2] = {100, 200};
            for (int i = 0; i < 2; i++)
            {
                const auto& row = dt->RowMap[i].second;
                CHECK(row.Properties.size() == 1);
                if (!row.Properties.empty())
                {
                    CHECK(row.Properties[0].Name.Text() == "Value");
                    auto* ip = dynamic_cast<IntProperty*>(row.Properties[0].Tag.get());
                    CHECK(ip != nullptr);
                    if (ip) CHECK(ip->Value == expected[i]);
                }
            }
        }
    }

    // Re-fetching the export returns the same cached instance.
    CHECK(pkg->GetExportObject(0) == obj);

    if (g_failures == 0)
    {
        std::cout << "All data-table tests passed.\n";
        return 0;
    }
    std::cout << g_failures << " check(s) failed.\n";
    return 1;
}
