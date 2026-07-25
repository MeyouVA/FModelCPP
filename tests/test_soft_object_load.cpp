// End-to-end test for FSoftObjectPath::Load / TryLoad across packages. Two hand-built .uassets in one
// provider:
//   * /Game/Target: export[0] "MyThing" of class "DataTable" -> UDataTable, export[1] "SubObj" (base UObject).
//   * /Game/Source: export[0] "Holder" (base UObject) carrying a "Ref" SoftObjectProperty whose path is
//     "/Game/Target.MyThing" with sub-path "SubObj".
// Verifies: the parsed SoftObjectProperty resolves through the provider + walks its sub-path to "SubObj";
// a directly-constructed FSoftObjectPath (no sub-path) typed-loads "MyThing" as a UDataTable; and a bad path
// TryLoad fails gracefully.
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
#include "UE4/Assets/Objects/Properties/SoftObjectProperty.h"
#include "UE4/Objects/UObject/FSoftObjectPath.h"
#include "UE4/Objects/UObject/FName.h"
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
namespace UO = CUE4Parse::UE4::Objects::UObject;

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
        auto pkg = std::make_unique<Package>(*file->second, nullptr, nullptr, nullptr, this);
        auto* raw = pkg.get();
        _loaded.emplace(path, std::move(pkg));
        return raw;
    }

private:
    std::map<std::string, std::unique_ptr<FByteArchive>> _files;
    std::map<std::string, std::unique_ptr<Package>> _loaded;
};

int main()
{
    const int32_t UE4_AUTO = static_cast<int32_t>(EUnrealEngineObjectUE4Version::AUTOMATIC_VERSION);
    const VersionContainer VC(GAME_UE4_LATEST, ETexturePlatform::DesktopMobile, FPackageFileVersion(864, UE4_AUTO, 0));

    TestFileProvider provider(VC);

    // ---------------- /Game/Target: "MyThing" (DataTable) + "SubObj" (base UObject) ----------------
    {
        PackageBuilder b;
        b.Pool = {"None", "Core", "Class", "Package", "DataTable", "/Script/Engine", "Object", "MyThing", "SubObj"};
        enum : int32_t { T_None = 0, T_Core, T_Class, T_Package, T_DataTable, T_ScriptEngine, T_Object,
                         T_MyThing, T_SubObj };
        b.PackageName = "Target";
        b.ImportCount = 3;

        AppendImport(b.Imports, T_Core, T_Class,   -2, T_DataTable, T_None); // [0] class DataTable  (-1)
        AppendImport(b.Imports, T_Core, T_Package,  0, T_ScriptEngine, T_None); // [1] /Script/Engine (-2)
        AppendImport(b.Imports, T_Core, T_Class,   -2, T_Object,    T_None); // [2] class Object     (-3)

        std::vector<uint8_t> myThing;         // DataTable with no RowStruct -> empty RowMap
        AppendFName(myThing, T_None);         // properties terminator
        AppendLE<int32_t>(myThing, 0);        // hasObjectGuid = false

        std::vector<uint8_t> subObj;          // base UObject
        AppendFName(subObj, T_None);
        AppendLE<int32_t>(subObj, 0);

        const auto sz0 = static_cast<int64_t>(myThing.size());
        const auto sz1 = static_cast<int64_t>(subObj.size());
        b.ExportData.insert(b.ExportData.end(), myThing.begin(), myThing.end());
        b.ExportData.insert(b.ExportData.end(), subObj.begin(), subObj.end());

        auto buf = b.Assemble(2, [&](int32_t serialOffset) {
            std::vector<uint8_t> e;
            AppendExport(e, -1, T_MyThing, serialOffset,       sz0); // class DataTable
            AppendExport(e, -3, T_SubObj,  serialOffset + sz0, sz1); // class Object
            return e;
        });
        provider.AddPackage("/Game/Target", "Target.uasset", std::move(buf));
    }

    // ---------------- /Game/Source: "Holder" with a "Ref" SoftObjectProperty ----------------
    {
        PackageBuilder b;
        b.Pool = {"None", "Core", "Class", "Package", "Object", "/Script/CoreUObject", "Holder", "Ref",
                  "SoftObjectProperty", "/Game/Target.MyThing"};
        enum : int32_t { S_None = 0, S_Core, S_Class, S_Package, S_Object, S_ScriptCore, S_Holder, S_Ref,
                         S_SoftObjectProperty, S_TargetPath };
        b.PackageName = "Source";
        b.ImportCount = 2;

        AppendImport(b.Imports, S_Core, S_Class,   -2, S_Object,    S_None); // [0] class Object          (-1)
        AppendImport(b.Imports, S_Core, S_Package,  0, S_ScriptCore, S_None); // [1] /Script/CoreUObject   (-2)

        // A tagged SoftObjectProperty "Ref": value = FSoftObjectPath (AssetPathName FName + SubPathString FString).
        // Size = 8 (FName) + FString("SubObj") = 8 + (4 + 7) = 19.
        auto& e = b.ExportData;
        AppendFName(e, S_Ref);
        AppendFName(e, S_SoftObjectProperty);
        AppendLE<int32_t>(e, 19);          // Size
        AppendLE<int32_t>(e, 0);           // ArrayIndex
        e.push_back(0);                    // HasPropertyGuid = false (no TagData for SoftObjectProperty)
        AppendFName(e, S_TargetPath);      // FSoftObjectPath.AssetPathName -> "/Game/Target.MyThing"
        AppendFString(e, "SubObj");        // FSoftObjectPath.SubPathString
        AppendFName(e, S_None);            // properties terminator
        AppendLE<int32_t>(e, 0);           // hasObjectGuid = false

        const auto serialSize = static_cast<int64_t>(b.ExportData.size());
        auto buf = b.Assemble(1, [&](int32_t serialOffset) {
            std::vector<uint8_t> ex;
            AppendExport(ex, -1, S_Holder, serialOffset, serialSize);
            return ex;
        });
        provider.AddPackage("/Game/Source", "Source.uasset", std::move(buf));
    }

    // ---------------- resolve the parsed SoftObjectProperty (with sub-path) ----------------
    auto* src = dynamic_cast<Package*>(provider.TryLoadPackage("/Game/Source"));
    CHECK(src != nullptr);
    if (src)
    {
        auto* holder = src->GetExportObject(0);
        CHECK(holder != nullptr);
        CHECK(holder && holder->Owner == src); // UObject.Owner is now wired
        if (holder)
        {
            const SoftObjectProperty* ref = nullptr;
            for (const auto& tag : holder->Properties)
                if (tag.Name.Text() == "Ref")
                    ref = dynamic_cast<const SoftObjectProperty*>(tag.Tag.get());
            CHECK(ref != nullptr);
            if (ref)
            {
                CHECK(ref->Value.AssetPathName.Text() == "/Game/Target.MyThing");
                CHECK(ref->Value.SubPathString == "SubObj");
                CHECK(ref->Value.ToString() == "/Game/Target.MyThing:SubObj");

                // Load() follows Owner->provider, then walks the sub-path to "SubObj".
                UObject* resolved = nullptr;
                const bool ok = ref->Value.TryLoad(resolved);
                CHECK(ok);
                CHECK(resolved != nullptr);
                if (resolved) CHECK(resolved->Name == "SubObj");
            }
        }
    }

    // ---------------- directly-constructed path, typed load (no sub-path) ----------------
    if (src)
    {
        UO::FSoftObjectPath direct(UO::FName("/Game/Target.MyThing"), "", src);
        auto* dt = direct.Load<UDataTable>();
        CHECK(dt != nullptr);
        if (dt) CHECK(dt->Name == "MyThing");

        // Re-fetching the target export returns the same cached instance the soft path resolved to.
        auto* tgt = dynamic_cast<Package*>(provider.TryLoadPackage("/Game/Target"));
        CHECK(tgt != nullptr);
        if (tgt) CHECK(tgt->GetExportObject(0) == dt);
    }

    // ---------------- a missing path fails gracefully ----------------
    if (src)
    {
        UO::FSoftObjectPath missing(UO::FName("/Game/DoesNotExist.Nope"), "", src);
        UObject* out = nullptr;
        CHECK(!missing.TryLoad(out));
        CHECK(out == nullptr);
    }

    if (g_failures == 0)
    {
        std::cout << "All soft-object load tests passed.\n";
        return 0;
    }
    std::cout << g_failures << " check(s) failed.\n";
    return 1;
}
