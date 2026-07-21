// End-to-end test for the UCurveBase family (UCurveVector / UCurveLinearColor / UCurveFloat). Each export carries
// its FRichCurve(s) as top-level tagged StructProperty(RichCurve) values on the object; the concrete export pulls
// them into its FloatCurves array (positional) during Deserialize. Verifies the registry builds the concrete
// types and that each extracted FRichCurve evaluates correctly (exercising the S25 curve subsystem as real
// exports). UCurveFloat is an empty subclass -- its single curve stays in Properties, read directly here.
#include <array>
#include <cmath>
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
#include "UE4/Assets/Objects/FPropertyTag.h"
#include "UE4/Assets/Objects/FScriptStruct.h"
#include "UE4/Assets/Objects/FStructFallback.h"
#include "UE4/Assets/Objects/Properties/StructProperty.h"
#include "UE4/Objects/Engine/Curves/RichCurve.h"
#include "UE4/Objects/Engine/Curves/UCurveVector.h"
#include "UE4/Objects/Engine/Curves/UCurveLinearColor.h"
#include "UE4/Objects/Engine/Curves/UCurveFloat.h"
#include "UE4/Objects/UObject/FPackageFileSummary.h"
#include "UE4/Readers/FByteArchive.h"
#include "UE4/Versions/VersionContainer.h"
#include "UE4/Versions/ObjectVersion.h"

using namespace CUE4Parse::FileProvider;
using namespace CUE4Parse::UE4::Assets;
using namespace CUE4Parse::UE4::Assets::Exports;
using namespace CUE4Parse::UE4::Assets::Objects::Properties;
using namespace CUE4Parse::UE4::Objects::Engine::Curves;
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

// Shared name pool + indices.
enum : int32_t {
    C_None = 0, C_Core, C_Class, C_Package, C_ScriptEngine,
    C_CurveVector, C_CurveLinearColor, C_CurveFloat,
    C_MyCurveVector, C_MyCurveLinearColor, C_MyCurveFloat,
    C_StructProperty, C_RichCurve, C_Keys, C_ArrayProperty, C_RichCurveKey, C_Time, C_Value, C_FloatProperty,
    C_PropX, C_PropY, C_PropZ, C_PropR, C_PropG, C_PropB, C_PropA, C_FloatCurveProp, C_AdjustBrightness
};

// Appends a classic tagged FloatProperty (no TagData, no property guid).
static void AppendFloatProperty(std::vector<uint8_t>& buf, int32_t nameIdx, float value)
{
    AppendFName(buf, nameIdx);      // Name
    AppendFName(buf, C_FloatProperty); // Type
    AppendLE<int32_t>(buf, 4);      // Size
    AppendLE<int32_t>(buf, 0);      // ArrayIndex
    buf.push_back(0);               // HasPropertyGuid = false
    AppendLE<float>(buf, value);    // value
}

// Builds the FStructFallback body of a RichCurve: a single "Keys" ArrayProperty<StructProperty(RichCurveKey)>
// holding two linear keys [(t0,v0),(t1,v1)], terminated by a None tag.
static std::vector<uint8_t> BuildRichCurveFallback(float t0, float v0, float t1, float v1)
{
    std::vector<uint8_t> keysValue;
    AppendLE<int32_t>(keysValue, 2);              // element count
    AppendFName(keysValue, C_Keys);               // inline inner tag Name
    AppendFName(keysValue, C_StructProperty);     // inner tag PropertyType
    AppendLE<int32_t>(keysValue, 0);              // inner tag Size
    AppendLE<int32_t>(keysValue, 0);              // inner tag ArrayIndex
    AppendFName(keysValue, C_RichCurveKey);       // struct tag data: struct type
    for (int i = 0; i < 16; i++) keysValue.push_back(0); // struct guid
    keysValue.push_back(0);                       // inner tag guid flag
    const float kd[2][2] = {{t0, v0}, {t1, v1}};
    for (auto& e : kd)
    {
        AppendFloatProperty(keysValue, C_Time, e[0]);
        AppendFloatProperty(keysValue, C_Value, e[1]);
        AppendFName(keysValue, C_None);           // key FStructFallback terminator
    }

    std::vector<uint8_t> body;
    AppendFName(body, C_Keys);                    // Keys tag Name
    AppendFName(body, C_ArrayProperty);           // Type
    AppendLE<int32_t>(body, static_cast<int32_t>(keysValue.size())); // Size = value bytes
    AppendLE<int32_t>(body, 0);                   // ArrayIndex
    AppendFName(body, C_StructProperty);          // tag data: array inner type
    body.push_back(0);                            // guid flag
    body.insert(body.end(), keysValue.begin(), keysValue.end());
    AppendFName(body, C_None);                    // RichCurve FStructFallback terminator
    return body;
}

// Appends a top-level tagged StructProperty(RichCurve): tag (Name, "StructProperty", Size, ArrayIndex,
// tagData = StructType FName + 16-byte guid, hasPropertyGuid=0) + the FStructFallback value bytes.
static void AppendRichCurveStructProperty(std::vector<uint8_t>& buf, int32_t nameIdx,
                                          const std::vector<uint8_t>& value)
{
    AppendFName(buf, nameIdx);                     // Name
    AppendFName(buf, C_StructProperty);            // Type
    AppendLE<int32_t>(buf, static_cast<int32_t>(value.size())); // Size = value bytes
    AppendLE<int32_t>(buf, 0);                     // ArrayIndex
    AppendFName(buf, C_RichCurve);                 // tag data: StructType
    for (int i = 0; i < 16; i++) buf.push_back(0); // StructGuid
    buf.push_back(0);                              // HasPropertyGuid = false
    buf.insert(buf.end(), value.begin(), value.end());
}

int main()
{
    const int32_t UE4_AUTO = static_cast<int32_t>(EUnrealEngineObjectUE4Version::AUTOMATIC_VERSION);
    const VersionContainer VC(GAME_UE4_LATEST, ETexturePlatform::DesktopMobile, FPackageFileVersion(864, UE4_AUTO, 0));

    TestFileProvider provider(VC);

    {
        PackageBuilder b;
        b.Pool = {"None", "Core", "Class", "Package", "/Script/Engine",
                  "CurveVector", "CurveLinearColor", "CurveFloat",
                  "MyCurveVector", "MyCurveLinearColor", "MyCurveFloat",
                  "StructProperty", "RichCurve", "Keys", "ArrayProperty", "RichCurveKey", "Time", "Value", "FloatProperty",
                  "X", "Y", "Z", "R", "G", "B", "A", "FloatCurve", "AdjustBrightness"};
        b.PackageName = "CurveAssetsPkg";
        b.ImportCount = 4;

        AppendImport(b.Imports, C_Core, C_Package,  0, C_ScriptEngine,     C_None); // [0] /Script/Engine (ref -1)
        AppendImport(b.Imports, C_Core, C_Class,   -1, C_CurveVector,      C_None); // [1] class CurveVector (ref -2)
        AppendImport(b.Imports, C_Core, C_Class,   -1, C_CurveLinearColor, C_None); // [2] class CurveLinearColor (ref -3)
        AppendImport(b.Imports, C_Core, C_Class,   -1, C_CurveFloat,       C_None); // [3] class CurveFloat (ref -4)

        struct Blob { int64_t off; int64_t size; };
        std::vector<Blob> blobs;
        auto beginBlob = [&]() -> size_t { return b.ExportData.size(); };
        auto endBlob = [&](size_t start) { blobs.push_back({static_cast<int64_t>(start), static_cast<int64_t>(b.ExportData.size() - start)}); };

        // Export 0: UCurveVector -- three RichCurve StructProperties X/Y/Z.
        {
            size_t s = beginBlob();
            AppendRichCurveStructProperty(b.ExportData, C_PropX, BuildRichCurveFallback(0.0f, 10.0f, 2.0f, 30.0f)); // Eval(1)=20
            AppendRichCurveStructProperty(b.ExportData, C_PropY, BuildRichCurveFallback(0.0f,  0.0f, 2.0f,  4.0f)); // Eval(1)=2
            AppendRichCurveStructProperty(b.ExportData, C_PropZ, BuildRichCurveFallback(0.0f, -5.0f, 2.0f,  5.0f)); // Eval(1)=0
            AppendFName(b.ExportData, C_None);            // UObject properties terminator
            AppendLE<int32_t>(b.ExportData, 0);           // hasObjectGuid = false
            endBlob(s);
        }
        // Export 1: UCurveLinearColor -- four RichCurve StructProperties R/G/B/A + one Adjust scalar.
        {
            size_t s = beginBlob();
            AppendRichCurveStructProperty(b.ExportData, C_PropR, BuildRichCurveFallback(0.0f, 1.0f, 2.0f, 3.0f)); // Eval(1)=2
            AppendRichCurveStructProperty(b.ExportData, C_PropG, BuildRichCurveFallback(0.0f, 0.0f, 2.0f, 1.0f)); // Eval(1)=0.5
            AppendRichCurveStructProperty(b.ExportData, C_PropB, BuildRichCurveFallback(0.0f, 10.0f, 2.0f, 10.0f)); // Eval(1)=10
            AppendRichCurveStructProperty(b.ExportData, C_PropA, BuildRichCurveFallback(0.0f, 0.0f, 2.0f, 1.0f)); // Eval(1)=0.5
            AppendFloatProperty(b.ExportData, C_AdjustBrightness, 2.0f); // exercises GetFloatProp (unobserved: field is private)
            AppendFName(b.ExportData, C_None);
            AppendLE<int32_t>(b.ExportData, 0);
            endBlob(s);
        }
        // Export 2: UCurveFloat -- empty subclass; its single RichCurve stays in Properties.
        {
            size_t s = beginBlob();
            AppendRichCurveStructProperty(b.ExportData, C_FloatCurveProp, BuildRichCurveFallback(0.0f, 7.0f, 2.0f, 9.0f)); // Eval(1)=8
            AppendFName(b.ExportData, C_None);
            AppendLE<int32_t>(b.ExportData, 0);
            endBlob(s);
        }

        const int32_t classIdx[3] = {-2, -3, -4};      // import[1], import[2], import[3]
        const int32_t objNames[3] = {C_MyCurveVector, C_MyCurveLinearColor, C_MyCurveFloat};
        auto buf = b.Assemble(3, [&](int32_t serialOffset) {
            std::vector<uint8_t> e;
            for (int i = 0; i < 3; i++)
                AppendExport(e, classIdx[i], objNames[i], serialOffset + static_cast<int32_t>(blobs[i].off), blobs[i].size);
            return e;
        });
        provider.AddPackage("/Game/CurveAssetsPkg", "CurveAssetsPkg.uasset", std::move(buf));
    }

    auto* pkg = dynamic_cast<Package*>(provider.TryLoadPackage("/Game/CurveAssetsPkg"));
    CHECK(pkg != nullptr);
    if (!pkg) { std::cout << g_failures << " check(s) failed.\n"; return 1; }

    // ---------- Export 0: UCurveVector ----------
    {
        auto* cv = dynamic_cast<UCurveVector*>(pkg->GetExportObject(0));
        CHECK(cv != nullptr); // registry produced the concrete type
        if (cv)
        {
            CHECK(cv->Name == "MyCurveVector");
            CHECK(cv->FloatCurves.size() == 3);
            CHECK(cv->FloatCurves[0].Keys.size() == 2);
            CHECK(cv->FloatCurves[1].Keys.size() == 2);
            CHECK(cv->FloatCurves[2].Keys.size() == 2);
            // Positional extraction: X/Y/Z land in FloatCurves[0/1/2]; linear midpoint at t=1.
            CHECK(std::fabs(cv->FloatCurves[0].Eval(1.0f) - 20.0f) < 1e-4f);
            CHECK(std::fabs(cv->FloatCurves[1].Eval(1.0f) -  2.0f) < 1e-4f);
            CHECK(std::fabs(cv->FloatCurves[2].Eval(1.0f) -  0.0f) < 1e-4f);
        }
    }

    // ---------- Export 1: UCurveLinearColor ----------
    {
        auto* cl = dynamic_cast<UCurveLinearColor*>(pkg->GetExportObject(1));
        CHECK(cl != nullptr);
        if (cl)
        {
            CHECK(cl->Name == "MyCurveLinearColor");
            CHECK(cl->FloatCurves.size() == 4);
            for (const auto& c : cl->FloatCurves) CHECK(c.Keys.size() == 2);
            CHECK(std::fabs(cl->FloatCurves[0].Eval(1.0f) -  2.0f) < 1e-4f); // R
            CHECK(std::fabs(cl->FloatCurves[1].Eval(1.0f) -  0.5f) < 1e-4f); // G
            CHECK(std::fabs(cl->FloatCurves[2].Eval(1.0f) - 10.0f) < 1e-4f); // B
            CHECK(std::fabs(cl->FloatCurves[3].Eval(1.0f) -  0.5f) < 1e-4f); // A
        }
    }

    // ---------- Export 2: UCurveFloat ----------
    {
        auto* cf = dynamic_cast<UCurveFloat*>(pkg->GetExportObject(2));
        CHECK(cf != nullptr); // registered concrete type (empty subclass)
        if (cf)
        {
            CHECK(cf->Name == "MyCurveFloat");
            // The single curve stays a tagged StructProperty on the object; build + eval it directly.
            CHECK(cf->Properties.size() == 1);
            if (!cf->Properties.empty())
            {
                auto* sp = dynamic_cast<StructProperty*>(cf->Properties[0].Tag.get());
                CHECK(sp != nullptr);
                if (sp && sp->Value.Struct)
                {
                    FRichCurve curve(*sp->Value.Struct);
                    CHECK(curve.Keys.size() == 2);
                    CHECK(std::fabs(curve.Eval(1.0f) - 8.0f) < 1e-4f);
                }
            }
        }
    }

    if (g_failures == 0)
    {
        std::cout << "All curve-asset tests passed.\n";
        return 0;
    }
    std::cout << g_failures << " check(s) failed.\n";
    return 1;
}
