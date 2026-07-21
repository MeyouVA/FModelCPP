// End-to-end test for UObjectRedirector and UBlueprintGeneratedClass. Two hand-built .uassets:
//   * RedirectorPkg: export "MyRedirector" of class "ObjectRedirector" -> UObjectRedirector, whose
//     DestinationObject is an FPackageIndex pointing at an import.
//   * BpgcPkg: export "MyBP_C" of class "BlueprintGeneratedClass" -> UBlueprintGeneratedClass, carrying
//     tagged properties (NumReplicatedProperties IntProperty, UberGraphFunction ObjectProperty,
//     ComponentTemplates ArrayProperty-of-ObjectProperty), a full UClass binary tail, and a cooked
//     EditorTags map. Verifies registry dispatch to the concrete types plus the field extraction.
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
#include "UE4/Assets/Exports/UObjectRedirector.h"
#include "UE4/Objects/Engine/UBlueprintGeneratedClass.h"
#include "UE4/Objects/UObject/FPackageFileSummary.h"
#include "UE4/Readers/FByteArchive.h"
#include "UE4/Versions/VersionContainer.h"
#include "UE4/Versions/ObjectVersion.h"

using namespace CUE4Parse::FileProvider;
using namespace CUE4Parse::UE4::Assets;
using namespace CUE4Parse::UE4::Assets::Exports;
using namespace CUE4Parse::UE4::Readers;
using namespace CUE4Parse::UE4::Versions;
namespace Engine = CUE4Parse::UE4::Objects::Engine;

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

// --- classic tagged-property writers (no TagData for scalar/object types, no property guid) ---
static void AppendIntProp(std::vector<uint8_t>& buf, int32_t nameIdx, int32_t intTypeIdx, int32_t value)
{
    AppendFName(buf, nameIdx);
    AppendFName(buf, intTypeIdx);   // "IntProperty"
    AppendLE<int32_t>(buf, 4);      // Size
    AppendLE<int32_t>(buf, 0);      // ArrayIndex
    buf.push_back(0);               // HasPropertyGuid = false
    AppendLE<int32_t>(buf, value);
}

static void AppendObjectProp(std::vector<uint8_t>& buf, int32_t nameIdx, int32_t objTypeIdx, int32_t index)
{
    AppendFName(buf, nameIdx);
    AppendFName(buf, objTypeIdx);   // "ObjectProperty"
    AppendLE<int32_t>(buf, 4);      // Size
    AppendLE<int32_t>(buf, 0);      // ArrayIndex
    buf.push_back(0);               // HasPropertyGuid = false
    AppendLE<int32_t>(buf, index);  // FPackageIndex
}

// An ArrayProperty whose inner type is ObjectProperty: TagData carries the inner-type FName, value is a
// count + that many int32 FPackageIndex elements.
static void AppendObjectArrayProp(std::vector<uint8_t>& buf, int32_t nameIdx, int32_t arrayTypeIdx,
                                  int32_t objTypeIdx, const std::vector<int32_t>& indices)
{
    const int32_t valueSize = static_cast<int32_t>(sizeof(int32_t) * (1 + indices.size())); // count + elements
    AppendFName(buf, nameIdx);
    AppendFName(buf, arrayTypeIdx); // "ArrayProperty"
    AppendLE<int32_t>(buf, valueSize);
    AppendLE<int32_t>(buf, 0);      // ArrayIndex
    AppendFName(buf, objTypeIdx);   // TagData.InnerType = "ObjectProperty"
    buf.push_back(0);               // HasPropertyGuid = false
    AppendLE<int32_t>(buf, static_cast<int32_t>(indices.size())); // element count
    for (int32_t idx : indices) AppendLE<int32_t>(buf, idx);
}

int main()
{
    const int32_t UE4_AUTO = static_cast<int32_t>(EUnrealEngineObjectUE4Version::AUTOMATIC_VERSION);
    const VersionContainer VC(GAME_UE4_LATEST, ETexturePlatform::DesktopMobile, FPackageFileVersion(864, UE4_AUTO, 0));

    TestFileProvider provider(VC);

    // ---------------- RedirectorPkg ----------------
    {
        PackageBuilder b;
        b.Pool = {"None", "Core", "Class", "Package", "ObjectRedirector", "/Script/CoreUObject",
                  "MyRedirector", "DestTarget"};
        enum : int32_t { R_None = 0, R_Core, R_Class, R_Package, R_ObjectRedirector, R_ScriptCore,
                         R_MyRedirector, R_DestTarget };
        b.PackageName = "RedirectorPkg";
        b.ImportCount = 3;

        AppendImport(b.Imports, R_Core, R_Class,   -2, R_ObjectRedirector, R_None); // [0] class ObjectRedirector
        AppendImport(b.Imports, R_Core, R_Package,  0, R_ScriptCore,       R_None); // [1] /Script/CoreUObject
        AppendImport(b.Imports, R_Core, R_Class,   -2, R_DestTarget,       R_None); // [2] the destination object

        AppendFName(b.ExportData, R_None);        // UObject: properties terminator
        AppendLE<int32_t>(b.ExportData, 0);       // UObject: hasObjectGuid = false
        AppendLE<int32_t>(b.ExportData, -3);      // DestinationObject -> import[2]

        const auto serialSize = static_cast<int64_t>(b.ExportData.size());
        auto buf = b.Assemble(1, [&](int32_t serialOffset) {
            std::vector<uint8_t> e;
            AppendExport(e, -1, R_MyRedirector, serialOffset, serialSize);
            return e;
        });
        provider.AddPackage("/Game/RedirectorPkg", "RedirectorPkg.uasset", std::move(buf));
    }

    // ---------------- BpgcPkg ----------------
    {
        PackageBuilder b;
        b.Pool = {"None", "Core", "Class", "Package", "BlueprintGeneratedClass", "/Script/Engine",
                  "MyBP_C", "NumReplicatedProperties", "IntProperty", "UberGraphFunction", "ObjectProperty",
                  "ComponentTemplates", "ArrayProperty", "UberGraph", "CompA", "CompB", "Foo"};
        enum : int32_t {
            G_None = 0, G_Core, G_Class, G_Package, G_BPGC, G_ScriptEngine, G_MyBP_C, G_NumRep, G_IntProperty,
            G_UberFunc, G_ObjectProperty, G_CompTemplates, G_ArrayProperty, G_UberGraph, G_CompA, G_CompB, G_Foo
        };
        b.PackageName = "BpgcPkg";
        b.ImportCount = 5;

        AppendImport(b.Imports, G_Core, G_Class,   -2, G_BPGC,      G_None); // [0] class BlueprintGeneratedClass
        AppendImport(b.Imports, G_Core, G_Package,  0, G_ScriptEngine, G_None); // [1] /Script/Engine
        AppendImport(b.Imports, G_Core, G_Class,   -2, G_UberGraph, G_None); // [2] UberGraph target  (-3)
        AppendImport(b.Imports, G_Core, G_Class,   -2, G_CompA,     G_None); // [3] component A       (-4)
        AppendImport(b.Imports, G_Core, G_Class,   -2, G_CompB,     G_None); // [4] component B       (-5)

        auto& e = b.ExportData;
        // UObject portion: tagged properties then None terminator + no object guid.
        AppendIntProp(e, G_NumRep, G_IntProperty, 3);
        AppendObjectProp(e, G_UberFunc, G_ObjectProperty, -3);
        AppendObjectArrayProp(e, G_CompTemplates, G_ArrayProperty, G_ObjectProperty, {-4, -5});
        AppendFName(e, G_None);            // properties terminator
        AppendLE<int32_t>(e, 0);           // hasObjectGuid = false
        // UStruct portion.
        AppendLE<int32_t>(e, 0);           // SuperStruct = null
        AppendLE<int32_t>(e, 0);           // Children count
        AppendLE<int32_t>(e, 0);           // ChildProperties count
        AppendLE<int32_t>(e, 0);           // bytecodeBufferSize
        AppendLE<int32_t>(e, 0);           // serializedScriptSize
        // UClass portion.
        AppendLE<int32_t>(e, 0);           // FuncMap count
        AppendLE<uint32_t>(e, 0);          // ClassFlags
        AppendLE<int32_t>(e, 0);           // ClassWithin = null
        AppendFName(e, G_None);            // ClassConfigName = None
        AppendLE<int32_t>(e, 0);           // ClassGeneratedBy = null
        AppendLE<int32_t>(e, 0);           // Interfaces count
        AppendLE<int32_t>(e, 0);           // discarded ReadBoolean (4 bytes)
        AppendFName(e, G_None);            // discarded ReadFName
        AppendLE<int32_t>(e, 1);           // bCooked (ReadBoolean, Ver >= ADD_COOKED_TO_UCLASS)
        AppendLE<int32_t>(e, 0);           // ClassDefaultObject = null
        // UBlueprintGeneratedClass portion: EditorTags map (1 pair). >4 bytes remain so the gate reads it.
        AppendLE<int32_t>(e, 1);           // map count
        AppendFName(e, G_Foo);             // key = "Foo"
        AppendFString(e, "Bar");           // value = "Bar"

        const auto serialSize = static_cast<int64_t>(b.ExportData.size());
        auto buf = b.Assemble(1, [&](int32_t serialOffset) {
            std::vector<uint8_t> ex;
            AppendExport(ex, -1, G_MyBP_C, serialOffset, serialSize);
            return ex;
        });
        provider.AddPackage("/Game/BpgcPkg", "BpgcPkg.uasset", std::move(buf));
    }

    // ---------------- UObjectRedirector ----------------
    {
        auto* pkg = dynamic_cast<Package*>(provider.TryLoadPackage("/Game/RedirectorPkg"));
        CHECK(pkg != nullptr);
        if (pkg)
        {
            auto* red = dynamic_cast<UObjectRedirector*>(pkg->GetExportObject(0));
            CHECK(red != nullptr); // registry produced the concrete type
            if (red)
            {
                CHECK(red->Name == "MyRedirector");
                CHECK(red->DestinationObject.IsImport());
                CHECK(red->DestinationObject.Index == -3);
                CHECK(red->DestinationObject.Name() == "DestTarget");
            }
        }
    }

    // ---------------- UBlueprintGeneratedClass ----------------
    {
        auto* pkg = dynamic_cast<Package*>(provider.TryLoadPackage("/Game/BpgcPkg"));
        CHECK(pkg != nullptr);
        if (pkg)
        {
            auto* bp = dynamic_cast<Engine::UBlueprintGeneratedClass*>(pkg->GetExportObject(0));
            CHECK(bp != nullptr); // registry produced the concrete type
            if (bp)
            {
                CHECK(bp->Name == "MyBP_C");
                // Base UClass binary parsed without desync.
                CHECK(bp->bCooked);
                CHECK(bp->ClassDefaultObject.IsNull());
                CHECK(bp->FuncMap.empty());
                // Tagged-property views.
                CHECK(bp->NumReplicatedProperties == 3);
                CHECK(bp->UberGraphFunction.has_value());
                if (bp->UberGraphFunction.has_value()) CHECK(bp->UberGraphFunction->Index == -3);
                CHECK(bp->SimpleConstructionScript == std::nullopt);
                CHECK(bp->ComponentTemplates.size() == 2);
                if (bp->ComponentTemplates.size() == 2)
                {
                    CHECK(bp->ComponentTemplates[0].Index == -4);
                    CHECK(bp->ComponentTemplates[1].Index == -5);
                    CHECK(bp->ComponentTemplates[0].Name() == "CompA");
                }
                CHECK(bp->DynamicBindingObjects.empty());
                // Custom-serialized EditorTags map.
                CHECK(bp->EditorTags.size() == 1);
                if (bp->EditorTags.size() == 1)
                {
                    CHECK(bp->EditorTags[0].first.Text() == "Foo");
                    CHECK(bp->EditorTags[0].second == "Bar");
                }
            }
        }
    }

    if (g_failures == 0)
    {
        std::cout << "All redirector / BPGC tests passed.\n";
        return 0;
    }
    std::cout << g_failures << " check(s) failed.\n";
    return 1;
}
