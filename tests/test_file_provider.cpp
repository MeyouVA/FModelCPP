// End-to-end test for the minimal IFileProvider: two hand-built .uasset packages served by an in-memory
// provider. Verifies cross-package import resolution (MainPkg's import resolves to the export inside
// OtherPkg and loads it), the "/Script/" fallback, provider caching, and FText Base localization through
// the provider's Internationalization table.
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
#include "UE4/Assets/ResolvedObject.h"
#include "UE4/Assets/Exports/UObject.h"
#include "UE4/Assets/Exports/EObjectFlags.h"
#include "UE4/Assets/Objects/FPropertyTag.h"
#include "UE4/Assets/Objects/Properties/IntProperty.h"
#include "UE4/Assets/Objects/Properties/TextProperty.h"
#include "UE4/Objects/Core/i18N/FText.h"
#include "UE4/Objects/UObject/FPackageFileSummary.h"
#include "UE4/Objects/UObject/ObjectResource.h"
#include "UE4/Readers/FByteArchive.h"
#include "UE4/Versions/VersionContainer.h"
#include "UE4/Versions/ObjectVersion.h"

using namespace CUE4Parse::FileProvider;
using namespace CUE4Parse::UE4::Assets;
using namespace CUE4Parse::UE4::Assets::Exports;
using namespace CUE4Parse::UE4::Assets::Objects::Properties;
using namespace CUE4Parse::UE4::Objects::UObject;
using namespace CUE4Parse::UE4::Readers;
using namespace CUE4Parse::UE4::Versions;
namespace i18N = CUE4Parse::UE4::Objects::Core::i18N;

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

// One FObjectImport record: ClassPackage, ClassName, OuterIndex, ObjectName, PackageName.
static void AppendImport(std::vector<uint8_t>& buf, int32_t classPackage, int32_t className,
                         int32_t outerIndex, int32_t objectName, int32_t packageName)
{
    AppendFName(buf, classPackage);
    AppendFName(buf, className);
    AppendLE<int32_t>(buf, outerIndex);
    AppendFName(buf, objectName);
    AppendFName(buf, packageName);
}

// One FObjectExport record (same layout as the other package tests).
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

// The FPackageFileSummary layout used by the other package tests.
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

// Assembles summary + names + imports + exports + export data with correct offsets.
struct PackageBuilder
{
    std::vector<std::string> Pool;
    std::vector<uint8_t> Imports;
    std::vector<uint8_t> ExportData;
    std::string PackageName;
    int32_t ImportCount = 0;

    // buildExports(serialOffset) — the export section given the export data's absolute offset.
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

// An in-memory provider: path -> archive, packages constructed (and owned) on first TryLoadPackage.
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
    provider.Internationalization.Override({{"NS", {{"KEY", "Bonjour"}}}});

    // ---------- OtherPkg: one root export "SharedObj" with an IntProperty Damage=7. ----------
    {
        PackageBuilder b;
        b.Pool = {"None", "SharedObj", "Damage", "IntProperty"};
        enum : int32_t { O_None = 0, O_SharedObj, O_Damage, O_IntProperty };
        b.PackageName = "OtherPkg";
        b.ImportCount = 0;

        AppendFName(b.ExportData, O_Damage);
        AppendFName(b.ExportData, O_IntProperty);
        AppendLE<int32_t>(b.ExportData, 4);  // Size
        AppendLE<int32_t>(b.ExportData, 0);  // ArrayIndex
        b.ExportData.push_back(0);           // guid flag
        AppendLE<int32_t>(b.ExportData, 7);  // value
        AppendFName(b.ExportData, O_None);   // terminator
        AppendLE<int32_t>(b.ExportData, 0);  // hasObjectGuid = false

        const auto serialSize = static_cast<int64_t>(b.ExportData.size());
        auto buf = b.Assemble(1, [&](int32_t serialOffset) {
            std::vector<uint8_t> e;
            AppendExport(e, 0, O_SharedObj, serialOffset, serialSize); // ClassIndex null
            return e;
        });
        provider.AddPackage("/Game/OtherPkg", "OtherPkg.uasset", std::move(buf));
    }

    // ---------- MainPkg: imports SharedObj (from /Game/OtherPkg) + EngineObj (from /Script/Engine),
    //            one export "MainObj" with a localized TextProperty. ----------
    {
        PackageBuilder b;
        b.Pool = {"None", "Core", "Class", "Package", "SharedObj", "/Game/OtherPkg",
                  "/Script/Engine", "EngineObj", "MainObj", "TextProperty", "MyText"};
        enum : int32_t { M_None = 0, M_Core, M_Class, M_Package, M_SharedObj, M_GameOtherPkg,
                         M_ScriptEngine, M_EngineObj, M_MainObj, M_TextProperty, M_MyText };
        b.PackageName = "MainPkg";
        b.ImportCount = 4;

        AppendImport(b.Imports, M_Core, M_Class,   -2, M_SharedObj,     M_None); // [0] -> outer import[1]
        AppendImport(b.Imports, M_Core, M_Package,  0, M_GameOtherPkg,  M_None); // [1] the other package
        AppendImport(b.Imports, M_Core, M_Package,  0, M_ScriptEngine,  M_None); // [2] a script package
        AppendImport(b.Imports, M_Core, M_Class,   -3, M_EngineObj,     M_None); // [3] -> outer import[2]

        // Export data: TextProperty "MyText" with a Base history (NS/KEY/SourceStr + dev notes).
        {
            std::vector<uint8_t> value;
            AppendLE<uint32_t>(value, 0);                     // Flags
            AppendLE<int8_t>(value, 0);                       // HistoryType = Base
            AppendFString(value, "NS");
            AppendFString(value, "KEY");
            AppendFString(value, "SourceStr");
            AppendFString(value, "dev");                      // dev notes (read: !IsFilterEditorOnly)

            AppendFName(b.ExportData, M_MyText);
            AppendFName(b.ExportData, M_TextProperty);
            AppendLE<int32_t>(b.ExportData, static_cast<int32_t>(value.size()));
            AppendLE<int32_t>(b.ExportData, 0);               // ArrayIndex
            b.ExportData.push_back(0);                        // guid flag
            b.ExportData.insert(b.ExportData.end(), value.begin(), value.end());
            AppendFName(b.ExportData, M_None);                // terminator
            AppendLE<int32_t>(b.ExportData, 0);               // hasObjectGuid = false
        }

        const auto serialSize = static_cast<int64_t>(b.ExportData.size());
        auto buf = b.Assemble(1, [&](int32_t serialOffset) {
            std::vector<uint8_t> e;
            AppendExport(e, -1, M_MainObj, serialOffset, serialSize); // ClassIndex -> import[0] SharedObj
            return e;
        });
        provider.AddPackage("/Game/MainPkg", "MainPkg.uasset", std::move(buf));
    }

    // ---------- load MainPkg through the provider ----------
    auto* mainPkg = dynamic_cast<Package*>(provider.TryLoadPackage("/Game/MainPkg"));
    CHECK(mainPkg != nullptr);
    if (!mainPkg) { std::cout << g_failures << " check(s) failed.\n"; return 1; }

    CHECK(mainPkg->GetProvider() == &provider);
    CHECK(provider.TryLoadPackage("/Game/MainPkg") == mainPkg); // cached
    CHECK(provider.TryLoadPackage("/Game/Nope") == nullptr);

    // The export's class index resolved cross-package during the header read.
    CHECK(mainPkg->ExportMap[0].ClassName == "SharedObj");

    // ---------- cross-package import resolution ----------
    auto* otherPkg = dynamic_cast<Package*>(provider.TryLoadPackage("/Game/OtherPkg"));
    CHECK(otherPkg != nullptr);
    {
        FPackageIndex impIdx(mainPkg, -1); // import[0] "SharedObj"
        ResolvedObject* r = mainPkg->ResolvePackageIndex(&impIdx);
        CHECK(r != nullptr);
        if (r && otherPkg)
        {
            CHECK(r->Name().Text() == "SharedObj");
            // It is the export inside OtherPkg (the fallback would be "/Game/OtherPkg.SharedObj").
            CHECK(r->GetPathName() == "OtherPkg.SharedObj");
            FPackageIndex expIdx(otherPkg, 1);
            CHECK(otherPkg->ResolvePackageIndex(&expIdx) == r);

            // And it loads: the object in the OTHER package, with its own properties.
            UObject* shared = r->Load<UObject>();
            CHECK(shared != nullptr);
            if (shared)
            {
                CHECK(shared->Name == "SharedObj");
                CHECK(shared->Properties.size() == 1);
                auto* damage = shared->Properties.empty() ? nullptr
                    : dynamic_cast<IntProperty*>(shared->Properties[0].Tag.get());
                CHECK(damage != nullptr);
                if (damage) CHECK(damage->Value == 7);
                CHECK(shared == otherPkg->GetExportObject(0)); // same instance, cached there
            }
        }
    }

    // ---------- "/Script/" import: in-package fallback ----------
    {
        FPackageIndex impIdx(mainPkg, -4); // import[3] "EngineObj" (outermost = /Script/Engine)
        ResolvedObject* r = mainPkg->ResolvePackageIndex(&impIdx);
        CHECK(r != nullptr);
        if (r)
        {
            CHECK(r->Name().Text() == "EngineObj");
            CHECK(r->GetPathName() == "/Script/Engine.EngineObj");
            CHECK(r->Object() == nullptr); // fallback: nothing to load
        }
    }

    // ---------- FText Base localization through the provider ----------
    {
        UObject* mainObj = mainPkg->GetExportObject(0);
        CHECK(mainObj != nullptr);
        if (mainObj)
        {
            CHECK(mainObj->Properties.size() == 1);
            auto* textProp = mainObj->Properties.empty() ? nullptr
                : dynamic_cast<TextProperty*>(mainObj->Properties[0].Tag.get());
            CHECK(textProp != nullptr);
            if (textProp)
            {
                auto* base = dynamic_cast<i18N::FTextHistory::Base*>(textProp->Value.TextHistory.get());
                CHECK(base != nullptr);
                if (base)
                {
                    CHECK(base->Namespace == "NS");
                    CHECK(base->Key == "KEY");
                    CHECK(base->SourceString == "SourceStr");
                    CHECK(base->LocalizedString == "Bonjour");
                }
                CHECK(textProp->Value.Text() == "Bonjour");
            }
        }
    }

    // ---------- SafeGet fallbacks ----------
    CHECK(provider.GetLocalizedString("NS", "KEY", "d") == "Bonjour");
    CHECK(provider.GetLocalizedString("NS", "MISSING", "d") == "d");
    CHECK(provider.Internationalization.SafeGet("NOPE", "KEY") == "");

    if (g_failures == 0)
    {
        std::cout << "All file-provider tests passed.\n";
        return 0;
    }
    std::cout << g_failures << " check(s) failed.\n";
    return 1;
}
