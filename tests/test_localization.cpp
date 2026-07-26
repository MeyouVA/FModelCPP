// Tests the .locres / .locmeta readers and the culture machinery built on them.
//
// Every archive here is hand-built, so each format version is exercised on its own terms:
//   * Legacy      — no magic number, so the reader must rewind to 0 and read strings inline.
//   * Compact     — a string lookup table at a trailing offset, no per-entry hashes, RefCount stays -1.
//   * Optimized_CRC32 — pre-hashed namespaces/keys, an entries count to skip, and RefCount stealing.
// Plus a .locmeta at both ELocMetaVersion::Initial (no compiled cultures) and AddedIsUGC, and the
// "too new to load" throw.
//
// The culture half runs over an in-memory AbstractFileProvider: InitFromMeta seeds AvailableCultures,
// InitFromIni adds the CultureMappings aliases, ChangeCulture sweeps the mounted .locres files (checking
// that the (?!Engine) exclusion really does skip Engine/), and GetLanguageCode is checked against the
// per-game tables.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "FileProvider/AbstractFileProvider.h"
#include "FileProvider/InternationalizationDictionary.h"
#include "FileProvider/Objects/GameFile.h"
#include "UE4/Localization/FTextLocalizationMetaDataResource.h"
#include "UE4/Localization/FTextLocalizationResource.h"
#include "UE4/Objects/Core/i18N/ELocMetaVersion.h"
#include "UE4/Objects/Core/i18N/ELocResVersion.h"
#include "UE4/Readers/FByteArchive.h"
#include "UE4Config/Parsing/ConfigIni.h"

using namespace CUE4Parse::FileProvider;
using CUE4Parse::Compression::CompressionMethod;
using CUE4Parse::FileProvider::Objects::FByteBulkDataHeader;
using CUE4Parse::FileProvider::Objects::GameFile;
using CUE4Parse::UE4::Localization::FTextLocalizationMetaDataResource;
using CUE4Parse::UE4::Localization::FTextLocalizationResource;
using CUE4Parse::UE4::Objects::Core::i18N::ELocMetaVersion;
using CUE4Parse::UE4::Objects::Core::i18N::ELocResVersion;
using CUE4Parse::UE4::Readers::FArchive;
using CUE4Parse::UE4::Readers::FByteArchive;
using CUE4Parse::UE4::VirtualFileSystem::GameFileMap;
using CUE4Parse::UE4::Versions::ELanguage;
using UE4Config::Parsing::ConfigIni;

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL: %s (line %d)\n", #cond, __LINE__); ++g_failures; } } while (0)

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

// The .locres / .locmeta magic numbers, as four little-endian uint32s.
static void AppendLocResMagic(std::vector<uint8_t>& buf)
{
    AppendLE<uint32_t>(buf, 0x7574140Eu);
    AppendLE<uint32_t>(buf, 0xFC034A67u);
    AppendLE<uint32_t>(buf, 0x9D90154Au);
    AppendLE<uint32_t>(buf, 0x1B7F37C3u);
}

static void AppendLocMetaMagic(std::vector<uint8_t>& buf)
{
    AppendLE<uint32_t>(buf, 0xA14CEE4Fu);
    AppendLE<uint32_t>(buf, 0x83554868u);
    AppendLE<uint32_t>(buf, 0xBD464C6Cu);
    AppendLE<uint32_t>(buf, 0x7C50DA70u);
}

// ---------------------------------------------------------------------------------------------------
// .locres builders
// ---------------------------------------------------------------------------------------------------

// Legacy: no magic, so the file starts straight at the namespace count and every string is inline.
static std::vector<uint8_t> BuildLegacyLocRes()
{
    std::vector<uint8_t> buf;
    AppendLE<uint32_t>(buf, 1);       // namespaceCount
    AppendFString(buf, "MENU");       // namespace (no hash at this version)
    AppendLE<uint32_t>(buf, 2);       // keyCount
    AppendFString(buf, "PLAY");
    AppendLE<uint32_t>(buf, 0x11111111u); // FEntry.SourceStringHash
    AppendFString(buf, "Jouer");          // inline localized string
    AppendFString(buf, "QUIT");
    AppendLE<uint32_t>(buf, 0x22222222u);
    AppendFString(buf, "Quitter");
    // The reader reads a 16-byte FGuid before deciding this is legacy, so the file has to be at least that
    // long; a real legacy .locres always is. Pad to be safe.
    while (buf.size() < 16) buf.push_back(0);
    return buf;
}

// Compact / Optimized_CRC32 share a layout; the version decides which fields are present.
static std::vector<uint8_t> BuildLocRes(ELocResVersion version)
{
    const bool optimized = version >= ELocResVersion::Optimized_CRC32;

    // Body first (everything after the string-array offset), so the offset can be computed.
    std::vector<uint8_t> body;
    if (optimized) AppendLE<int32_t>(body, 2); // EntriesCount — read past, never used
    AppendLE<uint32_t>(body, 1);               // namespaceCount
    if (optimized) AppendLE<uint32_t>(body, 0xAABBCCDDu); // namespace hash
    AppendFString(body, "MENU");
    AppendLE<uint32_t>(body, 2);               // keyCount

    if (optimized) AppendLE<uint32_t>(body, 0x01u);
    AppendFString(body, "PLAY");
    AppendLE<uint32_t>(body, 0x11111111u);     // FEntry.SourceStringHash
    AppendLE<int32_t>(body, 0);                // index into the string table

    if (optimized) AppendLE<uint32_t>(body, 0x02u);
    AppendFString(body, "QUIT");
    AppendLE<uint32_t>(body, 0x22222222u);
    AppendLE<int32_t>(body, 1);

    std::vector<uint8_t> header;
    AppendLocResMagic(header);
    header.push_back(static_cast<uint8_t>(version));

    // offset field + body, then the table itself.
    const int64_t offset = static_cast<int64_t>(header.size() + sizeof(int64_t) + body.size());

    std::vector<uint8_t> buf = header;
    AppendLE<int64_t>(buf, offset);
    buf.insert(buf.end(), body.begin(), body.end());

    AppendLE<int32_t>(buf, 2); // string table count
    AppendFString(buf, "Jouer");
    if (optimized) AppendLE<int32_t>(buf, 1); // RefCount
    AppendFString(buf, "Quitter");
    if (optimized) AppendLE<int32_t>(buf, 3);
    return buf;
}

static std::vector<uint8_t> BuildLocMeta(ELocMetaVersion version)
{
    std::vector<uint8_t> buf;
    AppendLocMetaMagic(buf);
    buf.push_back(static_cast<uint8_t>(version));
    AppendFString(buf, "en");
    AppendFString(buf, "Game.locres");
    if (version >= ELocMetaVersion::AddedCompiledCultures)
    {
        AppendLE<int32_t>(buf, 3);
        AppendFString(buf, "en");
        AppendFString(buf, "fr");
        AppendFString(buf, "ja");
    }
    if (version >= ELocMetaVersion::AddedIsUGC) AppendLE<int32_t>(buf, 1); // UE bools serialize as int32
    return buf;
}

// ---------------------------------------------------------------------------------------------------
// provider fixture
// ---------------------------------------------------------------------------------------------------

class MemoryGameFile : public GameFile
{
public:
    MemoryGameFile(const std::string& path, std::vector<uint8_t> contents)
        : GameFile(path, static_cast<int64_t>(contents.size())), _contents(std::move(contents)) {}

    bool IsEncrypted() const override { return false; }
    CompressionMethod GetCompressionMethod() const override { return CompressionMethod::None; }
    std::vector<uint8_t> Read(const FByteBulkDataHeader*) override { return _contents; }
    std::unique_ptr<FArchive> CreateReader(const FByteBulkDataHeader*) override
    { return std::make_unique<FByteArchive>(Path(), _contents); }

private:
    std::vector<uint8_t> _contents;
};

class MemoryFileProvider : public AbstractFileProvider
{
public:
    void Add(const std::string& path, std::vector<uint8_t> contents)
    {
        GameFileMap map;
        map[path] = std::make_shared<MemoryGameFile>(path, std::move(contents));
        Files.AddFiles(std::move(map));
    }
};

// ---------------------------------------------------------------------------------------------------

static void TestLegacyLocRes()
{
    const auto bytes = BuildLegacyLocRes();
    FByteArchive Ar("Game/Content/Localization/Game/fr/Game.locres", bytes);
    const FTextLocalizationResource locres(Ar);

    CHECK(locres.Entries.size() == 1);
    if (locres.Entries.empty()) return;

    const auto& [namespce, keys] = locres.Entries[0];
    CHECK(namespce.Str == "MENU");
    CHECK(namespce.StrHash == 0); // no hash stored below Optimized_CRC32
    CHECK(keys.size() == 2);
    if (keys.size() != 2) return;

    CHECK(keys[0].first.Str == "PLAY");
    CHECK(keys[0].second.LocalizedString == "Jouer");
    CHECK(keys[0].second.SourceStringHash == 0x11111111u);
    CHECK(keys[0].second.LocResName == "Game/Content/Localization/Game/fr/Game.locres");
    CHECK(keys[1].first.Str == "QUIT");
    CHECK(keys[1].second.LocalizedString == "Quitter");
}

static void TestCompactLocRes()
{
    const auto bytes = BuildLocRes(ELocResVersion::Compact);
    FByteArchive Ar("Game.locres", bytes);
    const FTextLocalizationResource locres(Ar);

    CHECK(locres.Entries.size() == 1);
    if (locres.Entries.empty()) return;

    const auto& [namespce, keys] = locres.Entries[0];
    CHECK(namespce.Str == "MENU");
    CHECK(namespce.StrHash == 0); // Compact stores no hashes
    CHECK(keys.size() == 2);
    if (keys.size() != 2) return;

    // Both strings come out of the lookup table at the trailing offset.
    CHECK(keys[0].first.Str == "PLAY");
    CHECK(keys[0].second.LocalizedString == "Jouer");
    CHECK(keys[1].first.Str == "QUIT");
    CHECK(keys[1].second.LocalizedString == "Quitter");
}

static void TestOptimizedLocRes()
{
    const auto bytes = BuildLocRes(ELocResVersion::Optimized_CRC32);
    FByteArchive Ar("Game.locres", bytes);
    const FTextLocalizationResource locres(Ar);

    CHECK(locres.Entries.size() == 1);
    if (locres.Entries.empty()) return;

    const auto& [namespce, keys] = locres.Entries[0];
    CHECK(namespce.Str == "MENU");
    CHECK(namespce.StrHash == 0xAABBCCDDu); // the pre-hashed namespace
    CHECK(keys.size() == 2);
    if (keys.size() != 2) return;

    CHECK(keys[0].first.Str == "PLAY");
    CHECK(keys[0].first.StrHash == 0x01u);
    CHECK(keys[0].second.LocalizedString == "Jouer");
    CHECK(keys[1].first.Str == "QUIT");
    CHECK(keys[1].first.StrHash == 0x02u);
    CHECK(keys[1].second.LocalizedString == "Quitter");
}

// An index past the end of the string table must not throw: the entry is kept with no translation.
static void TestOutOfRangeStringIndex()
{
    auto bytes = BuildLocRes(ELocResVersion::Optimized_CRC32);
    // Rewrite the second entry's string index (the last int32 of the body, immediately before the
    // 4-byte table count + the two table entries) to something out of range.
    std::vector<uint8_t> tail;
    AppendLE<int32_t>(tail, 2);
    AppendFString(tail, "Jouer");
    AppendLE<int32_t>(tail, 1);
    AppendFString(tail, "Quitter");
    AppendLE<int32_t>(tail, 3);
    const size_t indexPos = bytes.size() - tail.size() - sizeof(int32_t);
    std::memcpy(bytes.data() + indexPos, "\x63\x00\x00\x00", 4); // 99

    FByteArchive Ar("Game.locres", bytes);
    const FTextLocalizationResource locres(Ar);
    CHECK(locres.Entries.size() == 1);
    if (locres.Entries.empty()) return;
    const auto& keys = locres.Entries[0].second;
    CHECK(keys.size() == 2);
    if (keys.size() != 2) return;
    CHECK(keys[0].second.LocalizedString == "Jouer");
    CHECK(keys[1].second.LocalizedString.empty()); // no translation, but still present
}

static void TestLocMeta()
{
    {
        const auto bytes = BuildLocMeta(ELocMetaVersion::AddedIsUGC);
        FByteArchive Ar("Game.locmeta", bytes);
        const FTextLocalizationMetaDataResource meta(Ar);
        CHECK(meta.NativeCulture == "en");
        CHECK(meta.NativeLocRes == "Game.locres");
        CHECK(meta.bHasCompiledCultures);
        CHECK(meta.CompiledCultures.size() == 3);
        CHECK(meta.bIsUGC);
    }
    {
        // Initial carries neither the culture list (null upstream) nor the UGC flag.
        const auto bytes = BuildLocMeta(ELocMetaVersion::Initial);
        FByteArchive Ar("Game.locmeta", bytes);
        const FTextLocalizationMetaDataResource meta(Ar);
        CHECK(meta.NativeCulture == "en");
        CHECK(!meta.bHasCompiledCultures);
        CHECK(meta.CompiledCultures.empty());
        CHECK(!meta.bIsUGC);
    }
    {
        // A version above Latest is refused outright, unlike .locres (which has per-game exemptions).
        auto bytes = BuildLocMeta(ELocMetaVersion::AddedIsUGC);
        bytes[16] = static_cast<uint8_t>(ELocMetaVersion::LatestPlusOne);
        FByteArchive Ar("Game.locmeta", bytes);
        bool threw = false;
        try { const FTextLocalizationMetaDataResource meta(Ar); }
        catch (const std::exception&) { threw = true; }
        CHECK(threw);
    }
}

static void TestCultureResolution()
{
    InternationalizationDictionary dict;

    const auto bytes = BuildLocMeta(ELocMetaVersion::AddedIsUGC);
    FByteArchive Ar("Game.locmeta", bytes);
    const FTextLocalizationMetaDataResource meta(Ar);
    dict.InitFromMeta(meta);
    CHECK(dict.AvailableCultures().size() == 3); // en, fr, ja

    std::string validated;
    CHECK(dict.TryGetCulture("fr", validated) && validated == "fr");
    CHECK(!dict.TryGetCulture("de", validated));

    // With a mapping in place, an unshipped culture resolves to the one it aliases.
    ConfigIni ini("DefaultGame");
    ini.Read("[Internationalization]\n+CultureMappings=\"de;fr\"\n+CultureMappings=\"pt;pt-BR\"\n"
             "+LocalizationPaths=%GAMEDIR%Content/Localization/Game\n");
    InternationalizationDictionary mapped;
    mapped.InitFromIni(ini);
    CHECK(mapped.CultureMappings().size() == 2);
    CHECK(mapped.LocalizationPaths().size() == 1);
    mapped.InitFromMeta(meta);
    CHECK(mapped.TryGetCulture("de", validated) && validated == "fr");
    CHECK(!mapped.TryGetCulture("pt", validated)); // pt-BR is aliased but not shipped
}

static void TestProviderChangeCulture()
{
    MemoryFileProvider provider;
    provider.Add("FortniteGame/FortniteGame.uproject", {});
    provider.Add("FortniteGame/Content/Localization/Game/fr/Game.locres",
                 BuildLocRes(ELocResVersion::Optimized_CRC32));
    // The (?!Engine) exclusion means this one is never swept, even though it sits under /fr/ too.
    provider.Add("Engine/Content/Localization/Engine/fr/Engine.locres", BuildLegacyLocRes());

    const auto bytes = BuildLocMeta(ELocMetaVersion::AddedIsUGC);
    FByteArchive Ar("Game.locmeta", bytes);
    const FTextLocalizationMetaDataResource meta(Ar);
    provider.Internationalization.InitFromMeta(meta);

    const int count = provider.LoadLocalization("fr");
    CHECK(count == 2);
    CHECK(provider.Internationalization.Culture() == "fr");
    CHECK(provider.Internationalization.SafeGet("MENU", "PLAY") == "Jouer");
    CHECK(provider.Internationalization.SafeGet("MENU", "QUIT") == "Quitter");
    CHECK(provider.Internationalization.SafeGet("MENU", "MISSING", "fallback") == "fallback");
    // The Engine .locres would have contributed these had it not been excluded.
    CHECK(provider.Internationalization.Count() == 2);

    // An unshipped culture throws out of ChangeCulture, and TryChangeCulture swallows it.
    bool threw = false;
    try { provider.ChangeCulture("de"); } catch (const std::exception&) { threw = true; }
    CHECK(threw);
    CHECK(!provider.TryChangeCulture("de"));
    CHECK(provider.TryChangeCulture("en")); // shipped, just has no .locres to load
    CHECK(provider.Internationalization.Count() == 0);
}

static void TestLanguageCodes()
{
    MemoryFileProvider fortnite;
    fortnite.Add("FortniteGame/FortniteGame.uproject", {});
    CHECK(fortnite.ProjectName() == "FortniteGame");
    CHECK(fortnite.GetLanguageCode(ELanguage::English) == "en");
    CHECK(fortnite.GetLanguageCode(ELanguage::SpanishLatin) == "es-419");
    CHECK(fortnite.GetLanguageCode(ELanguage::TraditionalChinese) == "zh-Hant");
    CHECK(fortnite.GetLanguageCode(ELanguage::Zulu) == "en"); // not shipped -> that table's default

    MemoryFileProvider ark;
    ark.Add("ShooterGame/ShooterGame.uproject", {});
    CHECK(ark.GetLanguageCode(ELanguage::English) == "en-US");
    CHECK(ark.GetLanguageCode(ELanguage::Chinese) == "zh-CN");
    CHECK(ark.GetLanguageCode(ELanguage::VietnameseVietnam) == "vi-VN");
    CHECK(ark.GetLanguageCode(ELanguage::Zulu) == "en-US");

    MemoryFileProvider borderlands;
    borderlands.Add("OakGame/OakGame.uproject", {});
    CHECK(borderlands.GetLanguageCode(ELanguage::Chinese) == "zh-Hans-CN");
    CHECK(borderlands.GetLanguageCode(ELanguage::TraditionalChinese) == "zh-Hant-TW");

    // Anything else falls to the generic table, which is the only one carrying BritishEnglish or Zulu.
    MemoryFileProvider other;
    other.Add("SomeGame/SomeGame.uproject", {});
    CHECK(other.GetLanguageCode(ELanguage::BritishEnglish) == "en-GB");
    CHECK(other.GetLanguageCode(ELanguage::Zulu) == "zu");
    CHECK(other.GetLanguageCode(ELanguage::Portuguese) == "pt");
    CHECK(other.GetLanguageCode(ELanguage::Chinese) == "zh");
}

// Every case below is supposed to parse, so an escaping exception is a failure with a name attached
// rather than an unhandled-exception abort with no output.
static void Run(const char* name, void (*test)())
{
    try { test(); }
    catch (const std::exception& e)
    {
        std::printf("FAIL: %s threw: %s\n", name, e.what());
        ++g_failures;
    }
}

int main()
{
    Run("legacy .locres", TestLegacyLocRes);
    Run("compact .locres", TestCompactLocRes);
    Run("optimized .locres", TestOptimizedLocRes);
    Run("out-of-range string index", TestOutOfRangeStringIndex);
    Run(".locmeta", TestLocMeta);
    Run("culture resolution", TestCultureResolution);
    Run("provider ChangeCulture", TestProviderChangeCulture);
    Run("language codes", TestLanguageCodes);

    if (g_failures == 0)
    {
        std::printf("All localization tests passed.\n");
        return 0;
    }
    std::printf("%d check(s) failed.\n", g_failures);
    return 1;
}
