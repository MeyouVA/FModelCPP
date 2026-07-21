// End-to-end test for UUserDefinedStruct and UUserDefinedEnum. Two hand-built .uassets:
//   * StructPkg: export "MyStruct" of class "UserDefinedStruct" -> UUserDefinedStruct. Carries a tagged
//     "Status" EnumProperty (UDSS_UpToDate), a full UStruct binary tail, a uint32 StructFlags, and a default
//     instance (one IntProperty "MyInt"=42). Verifies registry dispatch, Status parsing, StructFlags, and the
//     default-property block.
//   * EnumPkg: export "MyEnum" of class "UserDefinedEnum" -> UUserDefinedEnum. Verifies the subclass is
//     produced by the registry and inherits UEnum's Names/CppForm/UnderlyingType deserialization.
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
#include "UE4/Objects/Engine/UUserDefinedStruct.h"
#include "UE4/Objects/Engine/UUserDefinedEnum.h"
#include "UE4/Assets/Objects/Properties/IntProperty.h"
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
namespace Props = CUE4Parse::UE4::Assets::Objects::Properties;

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

// --- classic tagged-property writers ---
static void AppendIntProp(std::vector<uint8_t>& buf, int32_t nameIdx, int32_t intTypeIdx, int32_t value)
{
    AppendFName(buf, nameIdx);
    AppendFName(buf, intTypeIdx);   // "IntProperty"
    AppendLE<int32_t>(buf, 4);      // Size
    AppendLE<int32_t>(buf, 0);      // ArrayIndex
    buf.push_back(0);               // HasPropertyGuid = false
    AppendLE<int32_t>(buf, value);
}

// An EnumProperty: TagData carries the enum's FName; the value is the member FName (8 bytes).
static void AppendEnumProp(std::vector<uint8_t>& buf, int32_t nameIdx, int32_t enumTypeIdx,
                           int32_t enumNameIdx, int32_t valueIdx)
{
    AppendFName(buf, nameIdx);
    AppendFName(buf, enumTypeIdx);  // "EnumProperty"
    AppendLE<int32_t>(buf, 8);      // Size (an FName value = 8 bytes)
    AppendLE<int32_t>(buf, 0);      // ArrayIndex
    AppendFName(buf, enumNameIdx);  // TagData.EnumName
    buf.push_back(0);               // HasPropertyGuid = false
    AppendFName(buf, valueIdx);     // Value = member FName
}

int main()
{
    const int32_t UE4_AUTO = static_cast<int32_t>(EUnrealEngineObjectUE4Version::AUTOMATIC_VERSION);
    const VersionContainer VC(GAME_UE4_LATEST, ETexturePlatform::DesktopMobile, FPackageFileVersion(864, UE4_AUTO, 0));

    TestFileProvider provider(VC);

    // ---------------- StructPkg ----------------
    {
        PackageBuilder b;
        b.Pool = {"None", "Core", "Class", "Package", "UserDefinedStruct", "/Script/Engine", "MyStruct",
                  "Status", "EnumProperty", "EUserDefinedStructureStatus",
                  "EUserDefinedStructureStatus::UDSS_UpToDate", "MyInt", "IntProperty"};
        enum : int32_t {
            S_None = 0, S_Core, S_Class, S_Package, S_UDS, S_ScriptEngine, S_MyStruct,
            S_Status, S_EnumProperty, S_EnumName, S_UpToDate, S_MyInt, S_IntProperty
        };
        b.PackageName = "StructPkg";
        b.ImportCount = 2;

        AppendImport(b.Imports, S_Core, S_Class,   -2, S_UDS,          S_None); // [0] class UserDefinedStruct
        AppendImport(b.Imports, S_Core, S_Package,  0, S_ScriptEngine, S_None); // [1] /Script/Engine

        auto& e = b.ExportData;
        // UObject portion: the "Status" tagged property (UDSS_UpToDate) then None terminator + no guid.
        AppendEnumProp(e, S_Status, S_EnumProperty, S_EnumName, S_UpToDate);
        AppendFName(e, S_None);             // properties terminator
        AppendLE<int32_t>(e, 0);            // hasObjectGuid = false
        // UStruct portion.
        AppendLE<int32_t>(e, 0);            // SuperStruct = null
        AppendLE<int32_t>(e, 0);            // Children count
        AppendLE<int32_t>(e, 0);            // ChildProperties count
        AppendLE<int32_t>(e, 0);            // bytecodeBufferSize
        AppendLE<int32_t>(e, 0);            // serializedScriptSize
        // UUserDefinedStruct portion.
        AppendLE<uint32_t>(e, 0x00000005u); // StructFlags
        // Default instance: one IntProperty then the None terminator (tagged, isStruct=true).
        AppendIntProp(e, S_MyInt, S_IntProperty, 42);
        AppendFName(e, S_None);             // default-properties terminator

        const auto serialSize = static_cast<int64_t>(b.ExportData.size());
        auto buf = b.Assemble(1, [&](int32_t serialOffset) {
            std::vector<uint8_t> ex;
            AppendExport(ex, -1, S_MyStruct, serialOffset, serialSize);
            return ex;
        });
        provider.AddPackage("/Game/StructPkg", "StructPkg.uasset", std::move(buf));
    }

    // ---------------- EnumPkg ----------------
    {
        PackageBuilder b;
        b.Pool = {"None", "Core", "Class", "Package", "UserDefinedEnum", "/Script/Engine", "MyEnum",
                  "MyEnum::A", "MyEnum::B"};
        enum : int32_t {
            E_None = 0, E_Core, E_Class, E_Package, E_UDE, E_ScriptEngine, E_MyEnum, E_A, E_B
        };
        b.PackageName = "EnumPkg";
        b.ImportCount = 2;

        AppendImport(b.Imports, E_Core, E_Class,   -2, E_UDE,          E_None); // [0] class UserDefinedEnum
        AppendImport(b.Imports, E_Core, E_Package,  0, E_ScriptEngine, E_None); // [1] /Script/Engine

        auto& e = b.ExportData;
        // UObject portion: no tagged props, just the None terminator + no guid.
        AppendFName(e, E_None);             // properties terminator
        AppendLE<int32_t>(e, 0);            // hasObjectGuid = false
        // UEnum portion: two (FName, int64) names, then CppForm + UnderlyingType bytes.
        AppendLE<int32_t>(e, 2);            // Names count
        AppendFName(e, E_A); AppendLE<int64_t>(e, 0);
        AppendFName(e, E_B); AppendLE<int64_t>(e, 1);
        e.push_back(2);                     // CppForm = EnumClass
        e.push_back(4);                     // UnderlyingType = uint8

        const auto serialSize = static_cast<int64_t>(b.ExportData.size());
        auto buf = b.Assemble(1, [&](int32_t serialOffset) {
            std::vector<uint8_t> ex;
            AppendExport(ex, -1, E_MyEnum, serialOffset, serialSize);
            return ex;
        });
        provider.AddPackage("/Game/EnumPkg", "EnumPkg.uasset", std::move(buf));
    }

    // ---------------- UUserDefinedStruct ----------------
    {
        auto* pkg = dynamic_cast<Package*>(provider.TryLoadPackage("/Game/StructPkg"));
        CHECK(pkg != nullptr);
        if (pkg)
        {
            auto* uds = dynamic_cast<Engine::UUserDefinedStruct*>(pkg->GetExportObject(0));
            CHECK(uds != nullptr); // registry produced the concrete type
            if (uds)
            {
                CHECK(uds->Name == "MyStruct");
                CHECK(uds->Status == Engine::EUserDefinedStructureStatus::UDSS_UpToDate);
                CHECK(uds->StructFlags == 0x00000005u);
                // UStruct tail parsed without desync.
                CHECK(uds->SuperStruct.IsNull());
                CHECK(uds->Children.empty());
                // Default instance read as a tagged property list.
                CHECK(uds->DefaultProperties.size() == 1);
                if (uds->DefaultProperties.size() == 1)
                {
                    CHECK(uds->DefaultProperties[0].Name.Text() == "MyInt");
                    auto* ip = dynamic_cast<const Props::IntProperty*>(uds->DefaultProperties[0].Tag.get());
                    CHECK(ip != nullptr);
                    if (ip) CHECK(ip->Value == 42);
                }
            }
        }
    }

    // ---------------- UUserDefinedEnum ----------------
    {
        auto* pkg = dynamic_cast<Package*>(provider.TryLoadPackage("/Game/EnumPkg"));
        CHECK(pkg != nullptr);
        if (pkg)
        {
            auto* ude = dynamic_cast<Engine::UUserDefinedEnum*>(pkg->GetExportObject(0));
            CHECK(ude != nullptr); // registry produced the concrete subclass
            if (ude)
            {
                CHECK(ude->Name == "MyEnum");
                CHECK(ude->Names.size() == 2);
                if (ude->Names.size() == 2)
                {
                    CHECK(ude->Names[0].first.Text() == "MyEnum::A");
                    CHECK(ude->Names[0].second == 0);
                    CHECK(ude->Names[1].first.Text() == "MyEnum::B");
                    CHECK(ude->Names[1].second == 1);
                }
                CHECK(ude->CppForm == CUE4Parse::UE4::Objects::UObject::UEnum::ECppForm::EnumClass);
                CHECK(ude->UnderlyingType == CUE4Parse::UE4::Objects::UObject::UEnum::EUnderlyingType::uint8);
            }
        }
    }

    if (g_failures == 0)
    {
        std::cout << "All user-defined struct / enum tests passed.\n";
        return 0;
    }
    std::cout << g_failures << " check(s) failed.\n";
    return 1;
}
