// End-to-end test for UStringTable + FText StringTableEntry localization through the provider.
// Two hand-built .uasset packages served by an in-memory provider:
//   * StringTablePkg: one export "MyStringTable" of class "StringTable" (import chain -> /Script/Engine), so
//     ConstructObject builds a UStringTable via ObjectTypeRegistry; its FStringTable has TABLE_NS + GREETING->Hello.
//   * StMainPkg: one export "MainObj" with a TextProperty whose history is a StringTableEntry referencing
//     "/Game/StringTablePkg.MyStringTable" / key "GREETING".
// Verifies the registry-driven construction, FStringTable read, provider->TryLoadPackageObject<UStringTable>,
// and that StringTableEntry resolves SourceString ("Hello") + LocalizedString ("Bonjour" via the dict).
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
#include "UE4/Assets/Exports/Internationalization/UStringTable.h"
#include "UE4/Assets/Objects/FPropertyTag.h"
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
using namespace CUE4Parse::UE4::Assets::Exports::Internationalization;
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
    provider.Internationalization.Override({{"TABLE_NS", {{"GREETING", "Bonjour"}}}});

    // ---------- StringTablePkg: export "MyStringTable" of class "StringTable" (import -> /Script/Engine). ----------
    {
        PackageBuilder b;
        b.Pool = {"None", "Core", "Class", "Package", "StringTable", "/Script/Engine", "MyStringTable"};
        enum : int32_t { S_None = 0, S_Core, S_Class, S_Package, S_StringTable, S_ScriptEngine, S_MyStringTable };
        b.PackageName = "StringTablePkg";
        b.ImportCount = 2;

        AppendImport(b.Imports, S_Core, S_Class,   -2, S_StringTable,  S_None); // [0] class, outer -> import[1]
        AppendImport(b.Imports, S_Core, S_Package,  0, S_ScriptEngine, S_None); // [1] the script package

        // Export data: (no tagged properties) None terminator, no ObjectGuid, then the FStringTable.
        AppendFName(b.ExportData, S_None);            // properties terminator
        AppendLE<int32_t>(b.ExportData, 0);           // hasObjectGuid = false
        AppendFString(b.ExportData, "TABLE_NS");      // FStringTable.TableNamespace
        AppendLE<int32_t>(b.ExportData, 1);           // KeysToEntries count
        AppendFString(b.ExportData, "GREETING");      //   key
        AppendFString(b.ExportData, "Hello");         //   value
        AppendLE<int32_t>(b.ExportData, 0);           // KeysToMetaData count

        const auto serialSize = static_cast<int64_t>(b.ExportData.size());
        auto buf = b.Assemble(1, [&](int32_t serialOffset) {
            std::vector<uint8_t> e;
            AppendExport(e, -1, S_MyStringTable, serialOffset, serialSize); // ClassIndex -> import[0] StringTable
            return e;
        });
        provider.AddPackage("/Game/StringTablePkg", "StringTablePkg.uasset", std::move(buf));
    }

    // ---------- StMainPkg: export "MainObj" with a StringTableEntry TextProperty. ----------
    {
        PackageBuilder b;
        b.Pool = {"None", "MainObj", "TextProperty", "MyText", "/Game/StringTablePkg.MyStringTable"};
        enum : int32_t { M_None = 0, M_MainObj, M_TextProperty, M_MyText, M_TablePath };
        b.PackageName = "StMainPkg";
        b.ImportCount = 0;

        // FText value: Flags, HistoryType = StringTableEntry (11), TableId FName, Key FString.
        std::vector<uint8_t> value;
        AppendLE<uint32_t>(value, 0);                 // Flags
        AppendLE<int8_t>(value, 11);                  // HistoryType = StringTableEntry
        AppendFName(value, M_TablePath);              // TableId -> "/Game/StringTablePkg.MyStringTable"
        AppendFString(value, "GREETING");             // Key

        AppendFName(b.ExportData, M_MyText);          // property Name
        AppendFName(b.ExportData, M_TextProperty);    // property Type
        AppendLE<int32_t>(b.ExportData, static_cast<int32_t>(value.size()));
        AppendLE<int32_t>(b.ExportData, 0);           // ArrayIndex
        b.ExportData.push_back(0);                    // HasPropertyGuid = false
        b.ExportData.insert(b.ExportData.end(), value.begin(), value.end());
        AppendFName(b.ExportData, M_None);            // properties terminator
        AppendLE<int32_t>(b.ExportData, 0);           // hasObjectGuid = false

        const auto serialSize = static_cast<int64_t>(b.ExportData.size());
        auto buf = b.Assemble(1, [&](int32_t serialOffset) {
            std::vector<uint8_t> e;
            AppendExport(e, 0, M_MainObj, serialOffset, serialSize); // ClassIndex null -> base UObject
            return e;
        });
        provider.AddPackage("/Game/StMainPkg", "StMainPkg.uasset", std::move(buf));
    }

    // ---------- the StringTable export builds as a UStringTable and reads its FStringTable ----------
    auto* stPkg = dynamic_cast<Package*>(provider.TryLoadPackage("/Game/StringTablePkg"));
    CHECK(stPkg != nullptr);
    if (!stPkg) { std::cout << g_failures << " check(s) failed.\n"; return 1; }
    {
        UObject* obj = stPkg->GetExportObject(0);
        CHECK(obj != nullptr);
        auto* table = dynamic_cast<UStringTable*>(obj);
        CHECK(table != nullptr); // ObjectTypeRegistry produced the concrete type
        if (table)
        {
            CHECK(table->Name == "MyStringTable");
            CHECK(table->StringTable.TableNamespace == "TABLE_NS");
            CHECK(table->StringTable.KeysToEntries.size() == 1);
            const auto it = table->StringTable.KeysToEntries.find("GREETING");
            CHECK(it != table->StringTable.KeysToEntries.end());
            if (it != table->StringTable.KeysToEntries.end()) CHECK(it->second == "Hello");
            CHECK(table->StringTable.KeysToMetaData.has_value());
        }
    }

    // ---------- provider->TryLoadPackageObject<T> directly ----------
    {
        auto* byPath = provider.TryLoadPackageObject<UStringTable>("/Game/StringTablePkg.MyStringTable");
        CHECK(byPath != nullptr);
        CHECK(byPath == stPkg->GetExportObject(0)); // same cached instance
        // Wrong path / missing object -> null (no throw).
        CHECK(provider.TryLoadPackageObject<UStringTable>("/Game/StringTablePkg.Nope") == nullptr);
        CHECK(provider.LoadPackageObject("/Game/Nope.Thing") == nullptr);
    }

    // ---------- StringTableEntry resolves SourceString + LocalizedString through the provider ----------
    {
        auto* mainPkg = dynamic_cast<Package*>(provider.TryLoadPackage("/Game/StMainPkg"));
        CHECK(mainPkg != nullptr);
        if (mainPkg)
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
                    auto* entry = dynamic_cast<i18N::FTextHistory::StringTableEntry*>(textProp->Value.TextHistory.get());
                    CHECK(entry != nullptr);
                    if (entry)
                    {
                        CHECK(entry->TableId.Text() == "/Game/StringTablePkg.MyStringTable");
                        CHECK(entry->Key == "GREETING");
                        CHECK(entry->SourceString == "Hello");       // from the string table entry
                        CHECK(entry->LocalizedString == "Bonjour");  // localized via the dictionary
                    }
                    CHECK(textProp->Value.Text() == "Bonjour");
                }
            }
        }
    }

    if (g_failures == 0)
    {
        std::cout << "All string-table tests passed.\n";
        return 0;
    }
    std::cout << g_failures << " check(s) failed.\n";
    return 1;
}
