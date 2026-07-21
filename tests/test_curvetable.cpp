// End-to-end test for UCurveTable: a hand-built .uasset with an export of class "CurveTable" whose body is
// numRows + a CurveTableMode byte + per-row (FName, FStructFallback). Verifies the registry builds the concrete
// UCurveTable, CurveTableMode is read, and each row's FStructFallback parses its tagged FloatProperty cell.
// A second export exercises the SimpleCurves mode; a third the Empty mode.
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "FileProvider/IFileProvider.h"
#include "FileProvider/InternationalizationDictionary.h"
#include "UE4/Assets/Package.h"
#include "UE4/Assets/Exports/UObject.h"
#include "UE4/Assets/Exports/EObjectFlags.h"
#include "UE4/Assets/Exports/Engine/UCurveTable.h"
#include "UE4/Objects/Engine/Curves/RealCurve.h"
#include "UE4/Objects/Engine/Curves/RichCurve.h"
#include "UE4/Assets/Objects/FPropertyTag.h"
#include "UE4/Assets/Objects/Properties/FloatProperty.h"
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

// Appends a classic tagged FloatProperty (no TagData, no property guid): Name, Type, Size, ArrayIndex, guid, value.
static void AppendFloatProperty(std::vector<uint8_t>& buf, int32_t nameIdx, int32_t typeIdx, float value)
{
    AppendFName(buf, nameIdx);      // Name
    AppendFName(buf, typeIdx);      // Type = "FloatProperty"
    AppendLE<int32_t>(buf, 4);      // Size
    AppendLE<int32_t>(buf, 0);      // ArrayIndex
    buf.push_back(0);               // HasPropertyGuid = false
    AppendLE<float>(buf, value);    // value
}

// Shared name pool + indices for all CurveTable exports.
enum : int32_t {
    C_None = 0, C_Core, C_Class, C_Package, C_CurveTable, C_ScriptEngine, C_MyCurveTable,
    C_DefaultValue, C_FloatProperty, C_Row0, C_Row1, C_Simple, C_Empty,
    C_EvalTable, C_RowA, C_Keys, C_ArrayProperty, C_StructProperty, C_RichCurveKey, C_Time, C_Value
};

int main()
{
    const int32_t UE4_AUTO = static_cast<int32_t>(EUnrealEngineObjectUE4Version::AUTOMATIC_VERSION);
    const VersionContainer VC(GAME_UE4_LATEST, ETexturePlatform::DesktopMobile, FPackageFileVersion(864, UE4_AUTO, 0));

    TestFileProvider provider(VC);

    // ---------- CurveTablePkg: three exports of class "CurveTable" (Rich / Simple / Empty modes). ----------
    {
        PackageBuilder b;
        b.Pool = {"None", "Core", "Class", "Package", "CurveTable", "/Script/Engine", "MyCurveTable",
                  "DefaultValue", "FloatProperty", "Row0", "Row1", "SimpleTable", "EmptyTable",
                  "EvalTable", "RowA", "Keys", "ArrayProperty", "StructProperty", "RichCurveKey", "Time", "Value"};
        b.PackageName = "CurveTablePkg";
        b.ImportCount = 2;

        AppendImport(b.Imports, C_Core, C_Class,   -2, C_CurveTable,   C_None); // [0] class CurveTable, outer -> import[1]
        AppendImport(b.Imports, C_Core, C_Package,  0, C_ScriptEngine, C_None); // [1] /Script/Engine

        // Build three export bodies back-to-back in ExportData, recording each (offset, size).
        struct Blob { int64_t off; int64_t size; };
        std::vector<Blob> blobs;

        auto beginBlob = [&]() -> size_t { return b.ExportData.size(); };
        auto endBlob = [&](size_t start) { blobs.push_back({static_cast<int64_t>(start), static_cast<int64_t>(b.ExportData.size() - start)}); };

        // Export 0: RichCurves, 2 rows, each a FStructFallback with a FloatProperty "DefaultValue".
        {
            size_t s = beginBlob();
            AppendFName(b.ExportData, C_None);            // UObject properties terminator (no tagged props)
            AppendLE<int32_t>(b.ExportData, 0);           // hasObjectGuid = false
            AppendLE<int32_t>(b.ExportData, 2);           // numRows
            b.ExportData.push_back(2);                    // CurveTableMode = RichCurves
            AppendFName(b.ExportData, C_Row0);
            AppendFloatProperty(b.ExportData, C_DefaultValue, C_FloatProperty, 1.5f);
            AppendFName(b.ExportData, C_None);            //   row 0 FStructFallback terminator
            AppendFName(b.ExportData, C_Row1);
            AppendFloatProperty(b.ExportData, C_DefaultValue, C_FloatProperty, 2.5f);
            AppendFName(b.ExportData, C_None);            //   row 1 FStructFallback terminator
            endBlob(s);
        }
        // Export 1: SimpleCurves, 1 row.
        {
            size_t s = beginBlob();
            AppendFName(b.ExportData, C_None);
            AppendLE<int32_t>(b.ExportData, 0);
            AppendLE<int32_t>(b.ExportData, 1);           // numRows
            b.ExportData.push_back(1);                    // CurveTableMode = SimpleCurves
            AppendFName(b.ExportData, C_Row0);
            AppendFloatProperty(b.ExportData, C_DefaultValue, C_FloatProperty, 9.0f);
            AppendFName(b.ExportData, C_None);
            endBlob(s);
        }
        // Export 2: Empty, 0 rows.
        {
            size_t s = beginBlob();
            AppendFName(b.ExportData, C_None);
            AppendLE<int32_t>(b.ExportData, 0);
            AppendLE<int32_t>(b.ExportData, 0);           // numRows
            b.ExportData.push_back(0);                    // CurveTableMode = Empty
            endBlob(s);
        }
        // Export 3: RichCurves, 1 row "RowA" whose FStructFallback carries a "Keys" array of 2 RichCurveKeys
        // [(Time=0,Value=10), (Time=2,Value=30)] with default (Linear) interp -> Eval(1) == 20.
        {
            size_t s = beginBlob();
            AppendFName(b.ExportData, C_None);            // UObject props terminator
            AppendLE<int32_t>(b.ExportData, 0);           // hasObjectGuid = false
            AppendLE<int32_t>(b.ExportData, 1);           // numRows
            b.ExportData.push_back(2);                    // CurveTableMode = RichCurves
            AppendFName(b.ExportData, C_RowA);            // row name

            // Build the "Keys" ArrayProperty<StructProperty(RichCurveKey)> value.
            std::vector<uint8_t> keysValue;
            AppendLE<int32_t>(keysValue, 2);              // element count
            // Inline inner tag (INNER_ARRAY_TAG_INFO): carries the element struct type descriptor.
            AppendFName(keysValue, C_Keys);               // inner tag Name
            AppendFName(keysValue, C_StructProperty);     // inner tag PropertyType
            AppendLE<int32_t>(keysValue, 0);              // inner tag Size
            AppendLE<int32_t>(keysValue, 0);              // inner tag ArrayIndex
            AppendFName(keysValue, C_RichCurveKey);       // struct tag data: struct type
            for (int i = 0; i < 16; i++) keysValue.push_back(0); // struct guid
            keysValue.push_back(0);                       // inner tag guid flag
            // Two element structs (each an FStructFallback: Time + Value FloatProperties + None).
            const float keyData[2][2] = {{0.0f, 10.0f}, {2.0f, 30.0f}};
            for (auto& kd : keyData)
            {
                AppendFloatProperty(keysValue, C_Time, C_FloatProperty, kd[0]);
                AppendFloatProperty(keysValue, C_Value, C_FloatProperty, kd[1]);
                AppendFName(keysValue, C_None);           // key FStructFallback terminator
            }
            // The Keys tag itself.
            AppendFName(b.ExportData, C_Keys);            // Name
            AppendFName(b.ExportData, C_ArrayProperty);   // Type
            AppendLE<int32_t>(b.ExportData, static_cast<int32_t>(keysValue.size())); // Size = value bytes
            AppendLE<int32_t>(b.ExportData, 0);           // ArrayIndex
            AppendFName(b.ExportData, C_StructProperty);  // tag data: array inner type
            b.ExportData.push_back(0);                    // guid flag
            b.ExportData.insert(b.ExportData.end(), keysValue.begin(), keysValue.end());
            AppendFName(b.ExportData, C_None);            // row FStructFallback terminator
            endBlob(s);
        }

        const int32_t objNames[4] = {C_MyCurveTable, C_Simple, C_Empty, C_EvalTable};
        auto buf = b.Assemble(4, [&](int32_t serialOffset) {
            std::vector<uint8_t> e;
            for (int i = 0; i < 4; i++)
                AppendExport(e, -1, objNames[i], serialOffset + static_cast<int32_t>(blobs[i].off), blobs[i].size);
            return e;
        });
        provider.AddPackage("/Game/CurveTablePkg", "CurveTablePkg.uasset", std::move(buf));
    }

    auto* pkg = dynamic_cast<Package*>(provider.TryLoadPackage("/Game/CurveTablePkg"));
    CHECK(pkg != nullptr);
    if (!pkg) { std::cout << g_failures << " check(s) failed.\n"; return 1; }

    // ---------- Export 0: RichCurves with 2 rows ----------
    {
        UObject* obj = pkg->GetExportObject(0);
        CHECK(obj != nullptr);
        auto* ct = dynamic_cast<UCurveTable*>(obj);
        CHECK(ct != nullptr); // ObjectTypeRegistry produced the concrete type
        if (ct)
        {
            CHECK(ct->Name == "MyCurveTable");
            CHECK(ct->CurveTableMode == ECurveTableMode::RichCurves);
            CHECK(ct->RowMap.size() == 2);
            if (ct->RowMap.size() == 2)
            {
                CHECK(ct->RowMap[0].first.Text() == "Row0");
                CHECK(ct->RowMap[1].first.Text() == "Row1");
                const float expected[2] = {1.5f, 2.5f};
                for (int i = 0; i < 2; i++)
                {
                    const auto& row = ct->RowMap[i].second;
                    CHECK(row.Properties.size() == 1);
                    if (!row.Properties.empty())
                    {
                        CHECK(row.Properties[0].Name.Text() == "DefaultValue");
                        auto* fp = dynamic_cast<FloatProperty*>(row.Properties[0].Tag.get());
                        CHECK(fp != nullptr);
                        if (fp) CHECK(fp->Value == expected[i]);
                    }
                }
            }
        }
        CHECK(pkg->GetExportObject(0) == obj); // cached
    }

    // ---------- Export 1: SimpleCurves with 1 row ----------
    {
        auto* ct = dynamic_cast<UCurveTable*>(pkg->GetExportObject(1));
        CHECK(ct != nullptr);
        if (ct)
        {
            CHECK(ct->CurveTableMode == ECurveTableMode::SimpleCurves);
            CHECK(ct->RowMap.size() == 1);
            if (ct->RowMap.size() == 1)
            {
                CHECK(ct->RowMap[0].first.Text() == "Row0");
                auto* fp = ct->RowMap[0].second.Properties.empty()
                    ? nullptr : dynamic_cast<FloatProperty*>(ct->RowMap[0].second.Properties[0].Tag.get());
                CHECK(fp != nullptr);
                if (fp) CHECK(fp->Value == 9.0f);
            }
        }
    }

    // ---------- Export 2: Empty, no rows ----------
    {
        auto* ct = dynamic_cast<UCurveTable*>(pkg->GetExportObject(2));
        CHECK(ct != nullptr);
        if (ct)
        {
            CHECK(ct->CurveTableMode == ECurveTableMode::Empty);
            CHECK(ct->RowMap.empty());
        }
    }

    // ---------- Export 3: RichCurve eval via FindCurve ----------
    {
        using namespace CUE4Parse::UE4::Objects::Engine::Curves;
        auto* ct = dynamic_cast<UCurveTable*>(pkg->GetExportObject(3));
        CHECK(ct != nullptr);
        if (ct)
        {
            CHECK(ct->CurveTableMode == ECurveTableMode::RichCurves);
            CHECK(ct->RowMap.size() == 1);
            if (ct->RowMap.size() == 1)
            {
                CHECK(ct->RowMap[0].first.Text() == "RowA");

                // FindCurve on the existing row builds an FRichCurve with the 2 parsed keys.
                auto curve = ct->FindCurve(ct->RowMap[0].first);
                CHECK(curve != nullptr);
                auto* rich = dynamic_cast<FRichCurve*>(curve.get());
                CHECK(rich != nullptr);
                if (rich)
                {
                    CHECK(rich->Keys.size() == 2);
                    if (rich->Keys.size() == 2)
                    {
                        CHECK(rich->Keys[0].Time == 0.0f);
                        CHECK(rich->Keys[0].Value == 10.0f);
                        CHECK(rich->Keys[1].Time == 2.0f);
                        CHECK(rich->Keys[1].Value == 30.0f);
                        CHECK(rich->Keys[0].InterpMode == ERichCurveInterpMode::RCIM_Linear);
                    }
                    // Linear interpolation midway between (0,10) and (2,30) -> 20.
                    CHECK(std::fabs(curve->Eval(1.0f) - 20.0f) < 1e-4f);
                    // At/after the last key -> last value (constant post-infinity default).
                    CHECK(std::fabs(curve->Eval(2.0f) - 30.0f) < 1e-4f);
                    CHECK(std::fabs(curve->Eval(5.0f) - 30.0f) < 1e-4f);
                    // Before/at the first key -> first value.
                    CHECK(std::fabs(curve->Eval(-1.0f) - 10.0f) < 1e-4f);
                }

                // NAME_None -> null; a name not present in the table -> null.
                CHECK(ct->FindCurve(FName()) == nullptr);
                CHECK(ct->FindCurve(FName(std::optional<std::string>("Ghost"), 99999)) == nullptr);

                // TryFindCurve mirrors FindCurve.
                std::unique_ptr<FRealCurve> out;
                CHECK(ct->TryFindCurve(ct->RowMap[0].first, out));
                CHECK(out != nullptr);
                CHECK(!ct->TryFindCurve(FName(), out));
                CHECK(out == nullptr);
            }
        }
    }

    if (g_failures == 0)
    {
        std::cout << "All curve-table tests passed.\n";
        return 0;
    }
    std::cout << g_failures << " check(s) failed.\n";
    return 1;
}
