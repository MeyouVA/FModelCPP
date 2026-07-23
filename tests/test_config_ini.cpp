// Tests the config-ini layer: the vendored UE4Config parser (sections, tokens, instruction kinds, line
// endings, FindPropertyInstructions), AbstractFileProvider::LoadIniConfigs / GameDisplayName /
// DefaultLightUnit / the internationalization tables it seeds, and AbstractVfsFileProvider::PostMount.
//
// The parser expectations below are ground truth: each fixture was run through the real
// Infrablack.UE4Config 0.7.2.97 assembly and the token stream recorded, so a divergence here is a
// divergence from what C# CUE4Parse actually sees.
//
// The provider fixtures are real version-8 paks authored byte-for-byte (the same writer as
// test_default_file_provider.cpp) mounted through the real PakFileReader, so LoadIniConfigs walks the
// genuine FixPath -> Files -> VfsEntry -> reader path.
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "FileProvider/AbstractFileProvider.h"
#include "FileProvider/Vfs/AbstractVfsFileProvider.h"
#include "UE4/Objects/Core/Misc/FGuid.h"
#include "UE4/Pak/Objects/FPakInfo.h"
#include "UE4/Readers/FByteArchive.h"
#include "UE4Config/Parsing/ConfigIni.h"

using namespace CUE4Parse::FileProvider;
using namespace CUE4Parse::FileProvider::Vfs;
using namespace UE4Config::Parsing;
using CUE4Parse::UE4::Objects::Core::Misc::FGuid;
using CUE4Parse::UE4::Objects::Engine::ELightUnits;
using CUE4Parse::UE4::Readers::FArchive;
using CUE4Parse::UE4::Readers::FByteArchive;
using CUE4Parse::UE4::Versions::VersionContainer;
using CUE4Parse::Utils::StringComparer;

static int g_failures = 0;
#define CHECK(cond)                                                           \
    do {                                                                      \
        if (!(cond)) {                                                        \
            std::cerr << "FAIL: " << #cond << " (line " << __LINE__ << ")\n"; \
            ++g_failures;                                                     \
        }                                                                     \
    } while (0)

// --------------------------------------------------------------------------------------------------
// Parser tests
// --------------------------------------------------------------------------------------------------

static const InstructionToken* AsInstruction(const IniToken* token)
{
    return dynamic_cast<const InstructionToken*>(token);
}

static void TestTokenClassification()
{
    ConfigIni ini("Test");
    ini.Read(
        "; leading comment\n"
        "# hash comment\n"
        "\n"
        "[/Script/EngineSettings.GeneralProjectSettings]\n"
        "ProjectName=MyGame\n"
        "+CulturesToStage=en\n"
        "-Removed=1\n"
        ".Dotted=2\n"
        "!ClearArray\n"
        "  SpacedKey = spaced value  \n"
        "JustText\n"
        "[Second Section]   ; waste suffix\n"
        "Key=Value=WithEquals\n");

    CHECK(ini.Sections.size() == 2);
    if (ini.Sections.size() != 2) return;

    // Everything before the first header lands in the unnamed section.
    const auto& preamble = *ini.Sections[0];
    CHECK(preamble.Name.empty());
    CHECK(preamble.Tokens.size() == 3);
    const auto* comment = dynamic_cast<const CommentToken*>(preamble.Tokens[0].get());
    CHECK(comment != nullptr && comment->Lines.size() == 1 && comment->Lines[0].Content == "; leading comment");
    // '#' is not a comment marker in this format, and a line without '=' is not an instruction.
    const auto* hash = dynamic_cast<const TextToken*>(preamble.Tokens[1].get());
    CHECK(hash != nullptr && hash->Text == "# hash comment");
    CHECK(dynamic_cast<const WhitespaceToken*>(preamble.Tokens[2].get()) != nullptr);

    const auto& settings = *ini.Sections[1];
    CHECK(settings.Name == "/Script/EngineSettings.GeneralProjectSettings");
    CHECK(settings.Tokens.size() == 9);
    if (settings.Tokens.size() != 9) return;

    const auto* set = AsInstruction(settings.Tokens[0].get());
    CHECK(set != nullptr && set->Type == InstructionType::Set && set->Key == "ProjectName" && set->Value == "MyGame");
    const auto* add = AsInstruction(settings.Tokens[1].get());
    CHECK(add != nullptr && add->Type == InstructionType::Add && add->Key == "CulturesToStage" && add->Value == "en");
    const auto* remove = AsInstruction(settings.Tokens[2].get());
    CHECK(remove != nullptr && remove->Type == InstructionType::Remove && remove->Key == "Removed");
    const auto* addForce = AsInstruction(settings.Tokens[3].get());
    CHECK(addForce != nullptr && addForce->Type == InstructionType::AddForce && addForce->Key == "Dotted");
    // RemoveAll takes the whole remainder as its key and never has a value.
    const auto* removeAll = AsInstruction(settings.Tokens[4].get());
    CHECK(removeAll != nullptr && removeAll->Type == InstructionType::RemoveAll &&
          removeAll->Key == "ClearArray" && removeAll->Value.empty());
    // Keys and values are not trimmed.
    const auto* spaced = AsInstruction(settings.Tokens[5].get());
    CHECK(spaced != nullptr && spaced->Key == "  SpacedKey " && spaced->Value == " spaced value  ");
    const auto* text = dynamic_cast<const TextToken*>(settings.Tokens[6].get());
    CHECK(text != nullptr && text->Text == "JustText");
    // A header line with trailing junk is not a header at all.
    const auto* notAHeader = dynamic_cast<const TextToken*>(settings.Tokens[7].get());
    CHECK(notAHeader != nullptr && notAHeader->Text == "[Second Section]   ; waste suffix");
    // ...so "Key=Value=WithEquals" is still in this section, split at the FIRST '='.
    const auto* firstEquals = AsInstruction(settings.Tokens[8].get());
    CHECK(firstEquals != nullptr && firstEquals->Key == "Key" && firstEquals->Value == "Value=WithEquals");
}

static void TestHeadersCommentsAndWhitespace()
{
    {
        ConfigIni ini("Test");
        ini.Read("  [Indented]  \nA=1\n");
        CHECK(ini.Sections.size() == 2);
        if (ini.Sections.size() < 2) return;
        CHECK(ini.Sections[1]->Name == "Indented");
        CHECK(ini.Sections[1]->LineWastePrefix == "  ");
        CHECK(ini.Sections[1]->LineWasteSuffix == "  ");
    }
    {
        // Consecutive comment lines merge into one token; a blank run merges into one whitespace token.
        ConfigIni ini("Test");
        ini.Read("[A]\n; c1\n; c2\n\t\n   \nX=1\n; c3\n");
        const auto& section = *ini.Sections[1];
        CHECK(section.Tokens.size() == 4);
        if (section.Tokens.size() != 4) return;
        const auto* comments = dynamic_cast<const CommentToken*>(section.Tokens[0].get());
        CHECK(comments != nullptr && comments->Lines.size() == 2);
        const auto* blanks = dynamic_cast<const WhitespaceToken*>(section.Tokens[1].get());
        CHECK(blanks != nullptr && blanks->Lines.size() == 2);
        CHECK(AsInstruction(section.Tokens[2].get()) != nullptr);
        CHECK(dynamic_cast<const CommentToken*>(section.Tokens[3].get()) != nullptr);
    }
    {
        // An indented comment is still a comment, but an indented instruction is not an instruction.
        ConfigIni ini("Test");
        ini.Read("[A]\n  ; indented\n  +Key=1\n");
        const auto& section = *ini.Sections[1];
        CHECK(section.Tokens.size() == 2);
        if (section.Tokens.size() != 2) return;
        CHECK(dynamic_cast<const CommentToken*>(section.Tokens[0].get()) != nullptr);
        const auto* set = AsInstruction(section.Tokens[1].get());
        CHECK(set != nullptr && set->Type == InstructionType::Set && set->Key == "  +Key");
    }
    {
        // '+' without a '=' is text; '=' without a key is a Set with an empty key; "[]" is a real section.
        ConfigIni ini("Test");
        ini.Read("+NoEquals\n=NoKey\n[]\n");
        CHECK(ini.Sections.size() == 2);
        if (ini.Sections.size() < 2) return;
        CHECK(ini.Sections[1]->Name.empty());
        const auto& preamble = *ini.Sections[0];
        CHECK(preamble.Tokens.size() == 2);
        if (preamble.Tokens.size() != 2) return;
        CHECK(dynamic_cast<const TextToken*>(preamble.Tokens[0].get()) != nullptr);
        const auto* noKey = AsInstruction(preamble.Tokens[1].get());
        CHECK(noKey != nullptr && noKey->Key.empty() && noKey->Value == "NoKey");
    }
}

static void TestLineEndings()
{
    ConfigIni ini("Test");
    ini.Read("[A]\r\nX=1\rY=2\nZ=3");
    CHECK(ini.Sections.size() == 2);
    if (ini.Sections.size() < 2) return;
    const auto& section = *ini.Sections[1];
    CHECK(section.Ending == LineEnding::Windows);
    CHECK(section.Tokens.size() == 3);
    if (section.Tokens.size() != 3) return;
    CHECK(AsInstruction(section.Tokens[0].get())->Ending == LineEnding::Mac);   // "X=1\r"
    CHECK(AsInstruction(section.Tokens[1].get())->Ending == LineEnding::Unix);  // "Y=2\n"
    CHECK(AsInstruction(section.Tokens[2].get())->Ending == LineEnding::None);  // no trailing ending
    CHECK(AsInstruction(section.Tokens[2].get())->Value == "3");
}

static void TestFindPropertyInstructions()
{
    ConfigIni ini("Test");
    ini.Read("[A]\nKey=1\n+Key=2\n!Key\n+Key=3\n[a]\nkey=lower\n[A]\nKey=second\n");

    // Every matching instruction, in file order, across every section with that name. Nothing resets the
    // list — not a Set, not a RemoveAll (verified against the real assembly).
    std::vector<const InstructionToken*> found;
    ini.FindPropertyInstructions("A", "Key", found);
    CHECK(found.size() == 5);
    if (found.size() != 5) return;
    CHECK(found[0]->Type == InstructionType::Set && found[0]->Value == "1");
    CHECK(found[1]->Type == InstructionType::Add && found[1]->Value == "2");
    CHECK(found[2]->Type == InstructionType::RemoveAll);
    CHECK(found[3]->Type == InstructionType::Add && found[3]->Value == "3");
    CHECK(found[4]->Type == InstructionType::Set && found[4]->Value == "second");

    // Section and key matching are both ordinal (case-sensitive).
    found.clear();
    ini.FindPropertyInstructions("a", "key", found);
    CHECK(found.size() == 1 && found[0]->Value == "lower");
    found.clear();
    ini.FindPropertyInstructions("A", "KEY", found);
    CHECK(found.empty());
    found.clear();
    ini.FindPropertyInstructions("Missing", "Key", found);
    CHECK(found.empty());

    // "!Key=Val" is a RemoveAll whose KEY is "Key=Val" — so it does not answer to "Key".
    ConfigIni other("Test");
    other.Read("[A]\n!Key=Val\n");
    found.clear();
    other.FindPropertyInstructions("A", "Key", found);
    CHECK(found.empty());
    CHECK(AsInstruction(other.Sections[1]->Tokens[0].get())->Key == "Key=Val");

    // Read() replaces the previous parse rather than appending to it.
    ini.Read("[B]\nOnly=1\n");
    CHECK(ini.Sections.size() == 2 && ini.FindSection("A") == nullptr && ini.FindSection("B") != nullptr);
}

// --------------------------------------------------------------------------------------------------
// Provider fixtures — a minimal version-8 pak writer (see test_default_file_provider.cpp)
// --------------------------------------------------------------------------------------------------

struct Writer
{
    std::vector<uint8_t> Bytes;

    int64_t Pos() const { return static_cast<int64_t>(Bytes.size()); }

    template <typename T>
    void Put(T value)
    {
        const auto* p = reinterpret_cast<const uint8_t*>(&value);
        Bytes.insert(Bytes.end(), p, p + sizeof(T));
    }

    void Raw(const std::string& data) { Bytes.insert(Bytes.end(), data.begin(), data.end()); }
    void Raw(const std::vector<uint8_t>& data) { Bytes.insert(Bytes.end(), data.begin(), data.end()); }
    void Zeros(int64_t count) { Bytes.insert(Bytes.end(), static_cast<size_t>(count), 0); }

    void FString(const std::string& s)
    {
        Put<int32_t>(static_cast<int32_t>(s.size() + 1));
        Raw(s);
        Put<uint8_t>(0);
    }
};

static constexpr int32_t kStoredStructSize = 8 + 8 + 8 + 4 + 20 + 1 + 4;

// One pak entry: path, contents, and whether its record carries FPakEntry's encrypted flag (which is what
// EncryptedFileCount counts — the pak index itself stays readable either way).
struct PakFile
{
    std::string Path;
    std::string Content;
    bool EntryEncrypted = false;
};

static std::vector<uint8_t> MakePak(const std::vector<PakFile>& files, const FGuid& encryptionKeyGuid = FGuid())
{
    Writer file;
    std::vector<int64_t> offsets;
    for (const auto& f : files)
    {
        offsets.push_back(file.Pos());
        file.Zeros(kStoredStructSize); // the duplicated record in front of the payload
        file.Raw(f.Content);
    }

    Writer index;
    index.FString("../../../Game/");
    index.Put<int32_t>(static_cast<int32_t>(files.size()));
    for (size_t i = 0; i < files.size(); ++i)
    {
        index.FString(files[i].Path);
        index.Put<int64_t>(offsets[i]);
        index.Put<int64_t>(static_cast<int64_t>(files[i].Content.size())); // compressed
        index.Put<int64_t>(static_cast<int64_t>(files[i].Content.size())); // uncompressed
        index.Put<int32_t>(0);  // method index: stored
        index.Zeros(20);        // hash
        index.Put<uint8_t>(files[i].EntryEncrypted ? 0x01 : 0x00);
        index.Put<uint32_t>(0); // compression block size
    }

    const int64_t indexOffset = file.Pos();
    file.Raw(index.Bytes);

    const int64_t trailerStart = file.Pos();
    file.Put<uint32_t>(encryptionKeyGuid.A);
    file.Put<uint32_t>(encryptionKeyGuid.B);
    file.Put<uint32_t>(encryptionKeyGuid.C);
    file.Put<uint32_t>(encryptionKeyGuid.D);
    file.Put<uint8_t>(0); // the index is not encrypted
    file.Put<uint32_t>(CUE4Parse::UE4::Pak::Objects::FPakInfo::PAK_FILE_MAGIC);
    file.Put<int32_t>(8);
    file.Put<int64_t>(indexOffset);
    file.Put<int64_t>(index.Pos());
    file.Zeros(20); // IndexHash
    for (int i = 0; i < 5; ++i) file.Zeros(32); // empty compression-method names
    CHECK(file.Pos() - trailerStart == 221);
    return file.Bytes;
}

class TestProvider : public AbstractVfsFileProvider
{
public:
    TestProvider() : AbstractVfsFileProvider(VersionContainer(), StringComparer::OrdinalIgnoreCase()) {}
    void Initialize() override {}
    using AbstractFileProvider::LoadIniConfigs; // protected in C#, needed directly by these tests
};

static std::shared_ptr<FArchive> Archive(std::string name, std::vector<uint8_t> bytes)
{
    return std::make_shared<FByteArchive>(std::move(name), std::move(bytes));
}

static const char* kDefaultGameIni =
    "[/Script/EngineSettings.GeneralProjectSettings]\n"
    "ProjectName=FallbackName\n"
    "ProjectDisplayedTitle=NSLOCTEXT(\"[ns]\", \"[key]\", \"My Great Game\")\n"
    "\n"
    "[/Script/UnrealEd.ProjectPackagingSettings]\n"
    "+CulturesToStage=en\n"
    "+CulturesToStage=fr\n"
    "CulturesToStage=ignored-because-it-is-a-Set\n"
    "\n"
    "[Internationalization]\n"
    "+CultureMappings=\"en-GB;en\"\n"
    "+LocalizationPaths=/Game/Content/Localization/Game\n";

static const char* kDefaultEngineIni =
    "[/Script/Engine.RendererSettings]\n"
    "r.DefaultFeature.LightUnits=2\n"
    "\n"
    "[ConsoleVariables]\n"
    "a.StripAdditiveRefPose=1\n";

static void TestLoadIniConfigs()
{
    TestProvider provider;
    provider.RegisterVfs(Archive("pakchunk0.pak", MakePak({
        {"Config/DefaultGame.ini", kDefaultGameIni},
        {"Config/DefaultEngine.ini", kDefaultEngineIni},
    })));
    CHECK(provider.Mount() == 1);
    CHECK(provider.Files.Count() == 2);

    // The inis are reachable at the canonical /Game/Config/... path through FixPath.
    CHECK(provider.TryGetGameFile("/Game/Config/DefaultGame.ini") != nullptr);

    CHECK(provider.LoadIniConfigs()); // DefaultGame has a GeneralProjectSettings section => the key works
    CHECK(provider.DefaultGame.FindSection("/Script/EngineSettings.GeneralProjectSettings") != nullptr);
    CHECK(provider.DefaultEngine.FindSection("/Script/Engine.RendererSettings") != nullptr);
    // Both inis came out of the same (unencrypted, zero-guid) pak.
    CHECK(provider.DefaultGame.EncryptionKeyGuid.has_value());
    CHECK(provider.DefaultEngine.EncryptionKeyGuid.has_value());

    // r.DefaultFeature.LightUnits=2 -> Lumens.
    CHECK(provider.DefaultLightUnit == ELightUnits::Lumens);

    // NSLOCTEXT(...) is unwrapped to its display string.
    CHECK(provider.GameDisplayName() == "My Great Game");

    // InitFromIni takes only the Add (+Key=) instructions.
    const auto& cultures = provider.Internationalization.AvailableCultures();
    CHECK(cultures.size() == 2 && cultures[0] == "en" && cultures[1] == "fr");
    const auto& mappings = provider.Internationalization.CultureMappings();
    CHECK(mappings.size() == 1 && mappings.count("en-GB") == 1 && mappings.at("en-GB") == "en");
    CHECK(provider.Internationalization.LocalizationPaths().size() == 1);
}

static void TestGameDisplayNameVariants()
{
    // INVTEXT and a bare value both unwrap; "{GameName}" falls back to ProjectName.
    const std::pair<const char*, const char*> cases[] = {
        {"ProjectDisplayedTitle=INVTEXT(\"Inv Title\")\n", "Inv Title"},
        {"ProjectDisplayedTitle=Plain Title\n", "Plain Title"},
        {"ProjectDisplayedTitle={GameName}\n", "FallbackName"},
        {"", "FallbackName"}, // no ProjectDisplayedTitle at all
    };

    for (const auto& [line, expected] : cases)
    {
        const std::string ini = std::string("[/Script/EngineSettings.GeneralProjectSettings]\n"
                                            "ProjectName=FallbackName\n") + line;
        TestProvider provider;
        provider.RegisterVfs(Archive("pakchunk0.pak", MakePak({{"Config/DefaultGame.ini", ini}})));
        provider.Mount();
        CHECK(provider.LoadIniConfigs());
        if (provider.GameDisplayName() != expected)
            std::cerr << "  (display name was \"" << provider.GameDisplayName() << "\", wanted \"" << expected << "\")\n";
        CHECK(provider.GameDisplayName() == expected);
    }
}

static void TestPostMountKeepsWorkingArchives()
{
    TestProvider provider;
    provider.RegisterVfs(Archive("pakchunk0.pak", MakePak({
        {"Config/DefaultGame.ini", kDefaultGameIni},
        {"secret.bin", "encrypted payload", true},
    }, FGuid(0x11111111u))));
    CHECK(provider.Mount() == 1);

    provider.PostMount();
    // The ini parsed into a GeneralProjectSettings section, so the key is right and nothing is dropped.
    CHECK(provider.MountedVfs().size() == 1);
    CHECK(provider.UnloadedVfs().empty());
    CHECK(provider.RequiredKeys().count(FGuid(0x11111111u)) == 0);
}

static void TestPostMountDropsArchivesWithABadKey()
{
    const FGuid guid(0x22222222u);
    TestProvider provider;
    // The ini decrypted to something that parses but has no GeneralProjectSettings section — C#'s signal
    // that the AES key was wrong.
    provider.RegisterVfs(Archive("pakchunk0.pak", MakePak({
        {"Config/DefaultGame.ini", "[SomeOtherSection]\nGarbage=1\n"},
        {"secret.bin", "encrypted payload", true},
    }, guid)));
    CHECK(provider.Mount() == 1);
    CHECK(provider.MountedVfs().size() == 1);

    int unmounted = 0;
    provider.VfsUnmounted = [&unmounted](CUE4Parse::UE4::VirtualFileSystem::IVfsReader&, int) { ++unmounted; };

    provider.PostMount();
    CHECK(unmounted == 1);
    CHECK(provider.MountedVfs().empty());
    CHECK(provider.UnloadedVfs().size() == 1);
    CHECK(provider.RequiredKeys().count(guid) == 1); // the guid goes back on the "still needed" list
}

static void TestPostMountWithoutInisDoesNothing()
{
    TestProvider provider;
    provider.RegisterVfs(Archive("pakchunk0.pak", MakePak({{"other.bin", "data", true}}, FGuid(0x33333333u))));
    CHECK(provider.Mount() == 1);

    // No DefaultGame.ini => EncryptionKeyGuid was never set => PostMount returns without touching anything.
    provider.PostMount();
    CHECK(!provider.DefaultGame.EncryptionKeyGuid.has_value());
    CHECK(provider.MountedVfs().size() == 1);
}

int main()
{
    try
    {
        TestTokenClassification();
        TestHeadersCommentsAndWhitespace();
        TestLineEndings();
        TestFindPropertyInstructions();
        TestLoadIniConfigs();
        TestGameDisplayNameVariants();
        TestPostMountKeepsWorkingArchives();
        TestPostMountDropsArchivesWithABadKey();
        TestPostMountWithoutInisDoesNothing();
    }
    catch (const std::exception& e)
    {
        std::cerr << "EXCEPTION: " << e.what() << "\n";
        return 2;
    }

    if (g_failures == 0)
    {
        std::cout << "test_config_ini: all checks passed\n";
        return 0;
    }
    std::cerr << g_failures << " check(s) failed.\n";
    return 1;
}
