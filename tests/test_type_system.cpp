// End-to-end test for the FProperties / UStruct type system. One hand-built .uasset with three exports:
//   * "MyStruct"   of class "ScriptStruct" -> UScriptStruct, with two ChildProperties (FIntProperty "Health",
//                  FBoolProperty "Flag") and StructFlags.
//   * "MyEnum"     of class "Enum"         -> UEnum, with two (FName, value) names, CppForm and UnderlyingType.
//   * "MyFunction" of class "Function"     -> UFunction, with FunctionFlags + event-graph fast-call fields.
// Verifies the ObjectTypeRegistry builds each concrete type and that FField::Construct dispatches the child
// properties to the right FProperty subclasses.
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
#include "UE4/Objects/UObject/UScriptStruct.h"
#include "UE4/Objects/UObject/UEnum.h"
#include "UE4/Objects/UObject/UFunction.h"
#include "UE4/Objects/UObject/UnrealType.h"
#include "UE4/Objects/UObject/FPackageFileSummary.h"
#include "UE4/Readers/FByteArchive.h"
#include "UE4/Versions/VersionContainer.h"
#include "UE4/Versions/ObjectVersion.h"

using namespace CUE4Parse::FileProvider;
using namespace CUE4Parse::UE4::Assets;
using namespace CUE4Parse::UE4::Assets::Exports;
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

// Name-pool indices.
enum : int32_t {
    N_None = 0, N_Core, N_Class, N_Package, N_ScriptStruct, N_Enum, N_Function, N_ScriptCore,
    N_MyStruct, N_MyEnum, N_MyFunction, N_Health, N_IntProperty, N_Flag, N_BoolProperty, N_A, N_B
};

// Writes the common FField + FProperty header for one ChildProperty (type name is read first by the loop).
static void AppendPropertyHeader(std::vector<uint8_t>& buf, int32_t typeIdx, int32_t nameIdx)
{
    AppendFName(buf, typeIdx);      // serialized field type name (read by the ChildProperties loop)
    AppendFName(buf, nameIdx);      // FField.Name
    AppendLE<uint32_t>(buf, 0);     // FField.Flags (EObjectFlags)
    AppendLE<int32_t>(buf, 1);      // FProperty.ArrayDim
    AppendLE<int32_t>(buf, 4);      // FProperty.ElementSize
    AppendLE<uint64_t>(buf, 0);     // FProperty.PropertyFlags
    AppendLE<uint16_t>(buf, 0);     // FProperty.RepIndex
    AppendFName(buf, N_None);       // FProperty.RepNotifyFunc = None
    buf.push_back(0);               // FProperty.BlueprintReplicationCondition (COND_None)
}

int main()
{
    const int32_t UE4_AUTO = static_cast<int32_t>(EUnrealEngineObjectUE4Version::AUTOMATIC_VERSION);
    const VersionContainer VC(GAME_UE4_LATEST, ETexturePlatform::DesktopMobile, FPackageFileVersion(864, UE4_AUTO, 0));

    TestFileProvider provider(VC);

    PackageBuilder b;
    b.Pool = {"None", "Core", "Class", "Package", "ScriptStruct", "Enum", "Function", "/Script/CoreUObject",
              "MyStruct", "MyEnum", "MyFunction", "Health", "IntProperty", "Flag", "BoolProperty", "A", "B"};
    b.PackageName = "TypesPkg";
    b.ImportCount = 4;

    AppendImport(b.Imports, N_Core, N_Class,   -4, N_ScriptStruct, N_None); // [0] class ScriptStruct -> import[3]
    AppendImport(b.Imports, N_Core, N_Class,   -4, N_Enum,         N_None); // [1] class Enum
    AppendImport(b.Imports, N_Core, N_Class,   -4, N_Function,     N_None); // [2] class Function
    AppendImport(b.Imports, N_Core, N_Package,  0, N_ScriptCore,   N_None); // [3] /Script/CoreUObject

    // --- Export 0: UScriptStruct "MyStruct" ---
    std::vector<uint8_t> structData;
    AppendFName(structData, N_None);              // UObject: properties terminator
    AppendLE<int32_t>(structData, 0);             // UObject: hasObjectGuid = false
    AppendLE<int32_t>(structData, 0);             // UStruct.SuperStruct = null
    AppendLE<int32_t>(structData, 0);             // UStruct.Children count = 0
    AppendLE<int32_t>(structData, 2);             // UStruct.ChildProperties count = 2
    AppendPropertyHeader(structData, N_IntProperty, N_Health);   // FIntProperty (no extra bytes)
    AppendPropertyHeader(structData, N_BoolProperty, N_Flag);    // FBoolProperty ...
    structData.push_back(1);                      //   FieldSize
    structData.push_back(0);                      //   ByteOffset
    structData.push_back(0xFF);                   //   ByteMask
    structData.push_back(0xFF);                   //   FieldMask
    structData.push_back(1);                      //   BoolSize
    structData.push_back(1);                      //   bIsNativeBool (ReadFlag)
    AppendLE<int32_t>(structData, 0);             // UStruct.bytecodeBufferSize
    AppendLE<int32_t>(structData, 0);             // UStruct.serializedScriptSize
    AppendLE<uint32_t>(structData, UO::STRUCT_Native); // UScriptStruct.StructFlags

    // --- Export 1: UEnum "MyEnum" ---
    std::vector<uint8_t> enumData;
    AppendFName(enumData, N_None);                // UObject: properties terminator
    AppendLE<int32_t>(enumData, 0);               // UObject: hasObjectGuid = false
    AppendLE<int32_t>(enumData, 2);               // UEnum.Names count = 2
    AppendFName(enumData, N_A); AppendLE<int64_t>(enumData, 0);
    AppendFName(enumData, N_B); AppendLE<int64_t>(enumData, 1);
    enumData.push_back(2);                        // CppForm = EnumClass
    enumData.push_back(4);                        // UnderlyingType = uint8

    // --- Export 2: UFunction "MyFunction" ---
    std::vector<uint8_t> funcData;
    AppendFName(funcData, N_None);                // UObject: properties terminator
    AppendLE<int32_t>(funcData, 0);               // UObject: hasObjectGuid = false
    AppendLE<int32_t>(funcData, 0);               // UStruct.SuperStruct = null
    AppendLE<int32_t>(funcData, 0);               // UStruct.Children count = 0
    AppendLE<int32_t>(funcData, 0);               // UStruct.ChildProperties count = 0
    AppendLE<int32_t>(funcData, 0);               // UStruct.bytecodeBufferSize
    AppendLE<int32_t>(funcData, 0);               // UStruct.serializedScriptSize
    AppendLE<uint32_t>(funcData, 0);              // UFunction.FunctionFlags (FUNC_None)
    AppendLE<int32_t>(funcData, 0);               // EventGraphFunction = null
    AppendLE<int32_t>(funcData, 0);               // EventGraphCallOffset

    const auto s0 = static_cast<int64_t>(structData.size());
    const auto s1 = static_cast<int64_t>(enumData.size());
    const auto s2 = static_cast<int64_t>(funcData.size());
    b.ExportData.insert(b.ExportData.end(), structData.begin(), structData.end());
    b.ExportData.insert(b.ExportData.end(), enumData.begin(), enumData.end());
    b.ExportData.insert(b.ExportData.end(), funcData.begin(), funcData.end());

    auto buf = b.Assemble(3, [&](int32_t serialOffset) {
        std::vector<uint8_t> e;
        AppendExport(e, -1, N_MyStruct,   serialOffset,           s0); // class import[0] ScriptStruct
        AppendExport(e, -2, N_MyEnum,     serialOffset + s0,      s1); // class import[1] Enum
        AppendExport(e, -3, N_MyFunction, serialOffset + s0 + s1, s2); // class import[2] Function
        return e;
    });
    provider.AddPackage("/Game/TypesPkg", "TypesPkg.uasset", std::move(buf));

    auto* pkg = dynamic_cast<Package*>(provider.TryLoadPackage("/Game/TypesPkg"));
    CHECK(pkg != nullptr);
    if (!pkg) { std::cout << g_failures << " check(s) failed.\n"; return 1; }

    // ---------- UScriptStruct with two child FProperties ----------
    {
        auto* ss = dynamic_cast<UO::UScriptStruct*>(pkg->GetExportObject(0));
        CHECK(ss != nullptr);
        if (ss)
        {
            CHECK(ss->Name == "MyStruct");
            CHECK(ss->StructFlags == UO::STRUCT_Native);
            CHECK(ss->SuperStruct.IsNull());
            CHECK(ss->Children.empty());
            CHECK(ss->ChildProperties.size() == 2);
            if (ss->ChildProperties.size() == 2)
            {
                auto* health = dynamic_cast<UO::FIntProperty*>(ss->ChildProperties[0].get());
                CHECK(health != nullptr);
                if (health) { CHECK(health->Name.Text() == "Health"); CHECK(health->ArrayDim == 1); }

                auto* flag = dynamic_cast<UO::FBoolProperty*>(ss->ChildProperties[1].get());
                CHECK(flag != nullptr);
                if (flag)
                {
                    CHECK(flag->Name.Text() == "Flag");
                    CHECK(flag->BoolSize == 1);
                    CHECK(flag->bIsNativeBool);
                    CHECK(flag->ByteMask == 0xFF);
                }
                // GetProperty finds by name.
                CHECK(ss->GetProperty(UO::FName("Flag")) == ss->ChildProperties[1].get());
            }
        }
    }

    // ---------- UEnum ----------
    {
        auto* en = dynamic_cast<UO::UEnum*>(pkg->GetExportObject(1));
        CHECK(en != nullptr);
        if (en)
        {
            CHECK(en->Name == "MyEnum");
            CHECK(en->Names.size() == 2);
            if (en->Names.size() == 2)
            {
                CHECK(en->Names[0].first.Text() == "A"); CHECK(en->Names[0].second == 0);
                CHECK(en->Names[1].first.Text() == "B"); CHECK(en->Names[1].second == 1);
            }
            CHECK(en->CppForm == UO::UEnum::ECppForm::EnumClass);
            CHECK(en->UnderlyingType == UO::UEnum::EUnderlyingType::uint8);
        }
    }

    // ---------- UFunction ----------
    {
        auto* fn = dynamic_cast<UO::UFunction*>(pkg->GetExportObject(2));
        CHECK(fn != nullptr);
        if (fn)
        {
            CHECK(fn->Name == "MyFunction");
            CHECK(fn->FunctionFlags == UO::FUNC_None);
            CHECK(fn->EventGraphFunction.has_value());
            if (fn->EventGraphFunction.has_value()) CHECK(fn->EventGraphFunction->IsNull());
            CHECK(fn->EventGraphCallOffset == 0);
        }
    }

    if (g_failures == 0)
    {
        std::cout << "All type-system tests passed.\n";
        return 0;
    }
    std::cout << g_failures << " check(s) failed.\n";
    return 1;
}
