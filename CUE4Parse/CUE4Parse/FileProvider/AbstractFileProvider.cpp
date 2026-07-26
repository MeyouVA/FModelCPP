#include "AbstractFileProvider.h"

#include <cctype>
#include <regex>
#include <stdexcept>

#include "Objects/OsGameFile.h"
#include "../UE4/VirtualFileSystem/IAesVfsReader.h"
#include "../UE4/VirtualFileSystem/VfsEntry.h"
#include "Vfs/IVfsFileProvider.h"
#include "../UE4/Assets/IoPackage.h"
#include "../UE4/Assets/Package.h"
#include "../UE4/Exceptions/ParserException.h"
#include "../UE4/IO/IoStoreReader.h"
#include "../UE4/IO/Objects/FIoStoreEntry.h"
#include "../UE4/Pak/Objects/FPakEntry.h"
#include "../UE4/Plugins/UPluginManifest.h"
#include "../UE4/Readers/FArchive.h"
#include "../Utils/Json.h"
#include "../Utils/StringUtils.h"

namespace CUE4Parse::FileProvider
{
    using Objects::GameFile;
    using UE4::Assets::IPackage;
    using UE4::Assets::Package;
    using UE4Config::Parsing::InstructionToken;

    namespace
    {
        // C#'s `new StreamReader(ar)` over an ini game file. Only UTF-8 (with or without a BOM) is decoded;
        // C#'s StreamReader would also detect a UTF-16 BOM, which no shipped ini has used so far. TODO.
        std::optional<std::string> ReadIniText(GameFile& file)
        {
            const auto bytes = file.SafeRead();
            if (!bytes.has_value()) return std::nullopt;

            const auto& data = *bytes;
            size_t start = 0;
            if (data.size() >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) start = 3;
            return std::string(reinterpret_cast<const char*>(data.data()) + start, data.size() - start);
        }

        bool EndsWithIgnoreCase(const std::string& s, const std::string& suffix)
        {
            if (s.size() < suffix.size()) return false;
            const size_t offset = s.size() - suffix.size();
            for (size_t i = 0; i < suffix.size(); ++i)
            {
                char a = s[offset + i];
                char b = suffix[i];
                if (a >= 'A' && a <= 'Z') a = static_cast<char>(a + 32);
                if (b >= 'A' && b <= 'Z') b = static_cast<char>(b + 32);
                if (a != b) return false;
            }
            return true;
        }

        // C#'s `new StreamReader(stream).ReadToEnd()` over a plugin descriptor. Any BOM is left in place
        // because Utils::Json::Parse strips it.
        std::string ReadAllText(UE4::Readers::FArchive& ar)
        {
            const int64_t remaining = ar.Length - ar.Position;
            if (remaining <= 0) return {};

            const auto bytes = ar.ReadBytes(static_cast<int>(remaining));
            return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        }

        // C#'s Regex.Escape, for splicing ProjectName into a pattern.
        std::string EscapeRegex(const std::string& s)
        {
            static const std::string specials = R"(\^$.|?*+()[]{})";
            std::string escaped;
            escaped.reserve(s.size());
            for (const char c : s)
            {
                if (specials.find(c) != std::string::npos) escaped.push_back('\\');
                escaped.push_back(c);
            }
            return escaped;
        }

        // C#'s string.Replace(old, ""), which removes every occurrence rather than just the first.
        std::string RemoveAll(std::string s, const std::string& fragment)
        {
            for (size_t at = s.find(fragment); at != std::string::npos; at = s.find(fragment, at))
                s.erase(at, fragment.size());
            return s;
        }

        // C#'s `private static readonly string[] pluginExtensions`. The third entry is misspelled upstream
        // ("Assetregisty.bin", no 'r' in "registry"), so no shipped file has ever ended with it and the
        // switch's default arm below is unreachable. Kept verbatim: fixing the spelling would start
        // populating VirtualPaths from asset registries, which is a behaviour change, not a port.
        const std::string pluginExtensions[] = {".uplugin", ".upluginmanifest", "Assetregisty.bin"};
    }

    AbstractFileProvider::AbstractFileProvider(UE4::Versions::VersionContainer versions,
                                               Utils::StringComparer pathComparer)
        : Versions(std::move(versions)), PathComparer(pathComparer), Internationalization(pathComparer),
          VirtualPaths(pathComparer), TextureCachePaths(pathComparer)
    {
    }

    const std::string& AbstractFileProvider::ProjectName() const
    {
        if (_projectName.empty())
        {
            const std::string* t = Files.FirstKey([](const std::string& it)
            {
                return EndsWithIgnoreCase(it, ".uproject");
            });
            if (t == nullptr)
            {
                t = Files.FirstKey([](const std::string& it)
                {
                    return !it.empty() && it[0] != '/' && it.find('/') != std::string::npos &&
                           !EndsWithIgnoreCase(Utils::SubstringBefore(it, '/'), "Engine");
                });
            }

            _projectName = t != nullptr ? Utils::SubstringBefore(*t, '/') : std::string();
            if (PathComparer.Equals(_projectName, "MidnightSuns"))
                _projectName = "CodaGame";
        }
        return _projectName;
    }

    std::string AbstractFileProvider::FixPath(const std::string& inPath) const
    {
        std::string path = inPath;
        for (auto& c : path) if (c == '\\') c = '/';
        if (path.empty()) return path; // C# would throw indexing path[0]; an empty path fixes to itself
        if (path[0] == '/') path.erase(0, 1);

        const std::string lastPart = Utils::SubstringAfterLast(path, '/');
        // This part is only for FSoftObjectPaths and not really needed anymore internally, but it's still
        // in here for user input
        if (lastPart.find('.') != std::string::npos &&
            Utils::SubstringBefore(lastPart, '.') == Utils::SubstringAfter(lastPart, '.'))
            path = Utils::SubstringBeforeWithLast(path, '/') + Utils::SubstringBefore(lastPart, '.');
        if (!path.empty() && path.back() != '/' && lastPart.find('.') == std::string::npos)
            path += "." + GameFile::UePackageExtensions[0]; // uasset

        std::string ret = path;
        const std::string root = Utils::SubstringBefore(path, '/');
        const std::string tree = Utils::SubstringAfter(path, '/');
        if (PathComparer.Equals(root, "Game") || PathComparer.Equals(root, "Engine"))
        {
            const std::string projectName = PathComparer.Equals(root, "Engine") ? "Engine" : ProjectName();
            const std::string root2 = Utils::SubstringBefore(tree, '/');
            if (PathComparer.Equals(root2, "Config") ||
                PathComparer.Equals(root2, "Content") ||
                PathComparer.Equals(root2, "Plugins"))
            {
                ret = projectName + '/' + tree;
            }
            else
            {
                ret = projectName + "/Content/" + tree;
            }
        }
        else if (PathComparer.Equals(root, ProjectName()))
        {
            // everything should be good
        }
        else if (const auto it = VirtualPaths.find(root); it != VirtualPaths.end())
        {
            ret = it->second + "/Content/" + tree;
        }
        else if (PathComparer.Equals(ProjectName(), "FortniteGame"))
        {
            ret = ProjectName() + "/Plugins/GameFeatures/" + root + "/Content/" + tree;
        }

        return ret;
    }

    std::shared_ptr<GameFile> AbstractFileProvider::TryGetGameFile(const std::string& path) const
    {
        const std::string fixedPath = FixPath(path);
        std::shared_ptr<GameFile> file;
        if (!Files.TryGetValue(fixedPath, file) && // any extension
            !Files.TryGetValue(Utils::SubstringBeforeWithLast(fixedPath, '.') + GameFile::UePackageExtensions[1], file) && // umap
            !Files.TryGetValue(path, file)) // in case FixPath broke something
        {
            file = nullptr;
        }
        return file;
    }

    std::shared_ptr<GameFile> AbstractFileProvider::TryGetGameFile(
        const std::string& path, const UE4::VirtualFileSystem::GameFileMap& collection) const
    {
        const std::string fixedPath = FixPath(path);
        const auto find = [&collection](const std::string& key) -> std::shared_ptr<GameFile>
        {
            const auto it = collection.find(key);
            return it != collection.end() ? it->second : nullptr;
        };
        auto file = find(fixedPath);
        if (file == nullptr) file = find(Utils::SubstringBeforeWithLast(fixedPath, '.') + GameFile::UePackageExtensions[1]);
        if (file == nullptr) file = find(path);
        return file;
    }

    std::shared_ptr<GameFile> AbstractFileProvider::GetGameFile(const std::string& path) const
    {
        auto file = TryGetGameFile(path);
        if (file == nullptr)
            throw std::out_of_range("There is no game file with the path \"" + path + "\"");
        return file;
    }

    std::optional<std::vector<uint8_t>> AbstractFileProvider::TrySaveAsset(const std::string& path) const
    {
        const auto file = TryGetGameFile(path);
        if (file == nullptr) return std::nullopt;
        return file->SafeRead();
    }

    std::unique_ptr<UE4::Readers::FArchive> AbstractFileProvider::TryCreateReader(const std::string& path) const
    {
        const auto file = TryGetGameFile(path);
        return file != nullptr ? file->SafeCreateReader() : nullptr;
    }

    const std::string& AbstractFileProvider::GameDisplayName() const
    {
        if (_gameDisplayName.empty())
        {
            std::vector<const InstructionToken*> instructions;
            DefaultGame.FindPropertyInstructions("/Script/EngineSettings.GeneralProjectSettings",
                                                 "ProjectDisplayedTitle", instructions);
            if (!instructions.empty())
            {
                // C#'s regex, with [\s\S] standing in for RegexOptions.Singleline's `.` and numbered groups
                // for its named 'target' captures (std::regex has no named groups).
                static const std::regex projectPattern(
                    R"rx(^(?:NSLOCTEXT\("[\s\S]*", "[\s\S]*", "([\s\S]*)"\)|INVTEXT\("([\s\S]*)"\)|([\s\S]*))$)rx");
                std::smatch match;
                const std::string& raw = instructions[0]->Value;
                if (std::regex_match(raw, match, projectPattern))
                {
                    std::string target;
                    for (size_t group = 1; group < match.size(); ++group)
                    {
                        if (match[group].matched) { target = match[group].str(); break; }
                    }

                    if (target.rfind("LOCTABLE(\"/Game/", 0) == 0)
                    {
                        // C# loads the UStringTable the title indirects through; object loading of string
                        // tables is not wired here yet, so the display name simply stays empty. TODO.
                    }
                    else if (target.find_first_not_of(" \t\r\n") != std::string::npos && target != "{GameName}")
                    {
                        _gameDisplayName = target;
                    }
                    else
                    {
                        instructions.clear();
                        DefaultGame.FindPropertyInstructions("/Script/EngineSettings.GeneralProjectSettings",
                                                             "ProjectName", instructions);
                        if (!instructions.empty()) _gameDisplayName = instructions[0]->Value;
                    }
                }
            }
            else
            {
                DefaultGame.FindPropertyInstructions("/Script/EngineSettings.GeneralProjectSettings",
                                                     "ProjectName", instructions);
                if (!instructions.empty()) _gameDisplayName = instructions[0]->Value;
            }
        }

        if (Versions.Game() == UE4::Versions::GAME_Back4Blood)
            _gameDisplayName = "Back 4 Blood"; // They left it as LDTEXT("TEXT_UI_GameTitle")

        return _gameDisplayName;
    }

    int AbstractFileProvider::LoadLocalization(const std::string& culture)
    {
        ChangeCulture(culture);
        return static_cast<int>(Internationalization.Count());
    }

    bool AbstractFileProvider::TryChangeCulture(const std::string& culture)
    {
        try
        {
            ChangeCulture(culture);
            return true;
        }
        catch (...) // C#'s bare `catch` — a bad culture and a bad .locres are both swallowed here
        {
            return false;
        }
    }

    std::string AbstractFileProvider::GetLanguageCode(UE4::Versions::ELanguage language) const
    {
        using UE4::Versions::ELanguage;

        // C#'s ProjectName.ToLowerInvariant() switch. Each game's table lists only the languages it ships;
        // anything else falls to that table's own default, which is why the defaults differ per game.
        std::string project = ProjectName();
        for (char& c : project) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        if (project == "fortnitegame")
        {
            switch (language)
            {
                case ELanguage::English:            return "en";
                case ELanguage::French:             return "fr";
                case ELanguage::German:             return "de";
                case ELanguage::Italian:            return "it";
                case ELanguage::Spanish:            return "es";
                case ELanguage::SpanishLatin:       return "es-419";
                case ELanguage::Arabic:             return "ar";
                case ELanguage::Japanese:           return "ja";
                case ELanguage::Korean:             return "ko";
                case ELanguage::Polish:             return "pl";
                case ELanguage::PortugueseBrazil:   return "pt-BR";
                case ELanguage::Russian:            return "ru";
                case ELanguage::Turkish:            return "tr";
                case ELanguage::Chinese:            return "zh-CN";
                case ELanguage::TraditionalChinese: return "zh-Hant";
                default:                            return "en";
            }
        }
        if (project == "worldexplorers")
        {
            switch (language)
            {
                case ELanguage::English:          return "en";
                case ELanguage::French:           return "fr";
                case ELanguage::German:           return "de";
                case ELanguage::Italian:          return "it";
                case ELanguage::Spanish:          return "es";
                case ELanguage::Japanese:         return "ja";
                case ELanguage::Korean:           return "ko";
                case ELanguage::PortugueseBrazil: return "pt-BR";
                case ELanguage::Russian:          return "ru";
                case ELanguage::Chinese:          return "zh-Hans";
                default:                          return "en";
            }
        }
        if (project == "shootergame")
        {
            switch (language)
            {
                case ELanguage::English:            return "en-US";
                case ELanguage::French:             return "fr-FR";
                case ELanguage::German:             return "de-DE";
                case ELanguage::Italian:            return "it-IT";
                case ELanguage::Spanish:            return "es-ES";
                case ELanguage::SpanishMexico:      return "es-MX";
                case ELanguage::Arabic:             return "ar-AE";
                case ELanguage::Japanese:           return "ja-JP";
                case ELanguage::Korean:             return "ko-KR";
                case ELanguage::Polish:             return "pl-PL";
                case ELanguage::PortugueseBrazil:   return "pt-BR";
                case ELanguage::Russian:            return "ru-RU";
                case ELanguage::Turkish:            return "tr-TR";
                case ELanguage::Chinese:            return "zh-CN";
                case ELanguage::TraditionalChinese: return "zh-TW";
                case ELanguage::Indonesian:         return "id-ID";
                case ELanguage::Thai:               return "th-TH";
                case ELanguage::VietnameseVietnam:  return "vi-VN";
                default:                            return "en-US";
            }
        }
        if (project == "stateofdecay2")
        {
            switch (language)
            {
                case ELanguage::English:           return "en-US";
                case ELanguage::AustralianEnglish: return "en-AU";
                case ELanguage::French:            return "fr-FR";
                case ELanguage::German:            return "de-DE";
                case ELanguage::Italian:           return "it-IT";
                case ELanguage::SpanishMexico:     return "es-MX";
                case ELanguage::PortugueseBrazil:  return "pt-BR";
                case ELanguage::Russian:           return "ru-RU";
                case ELanguage::Chinese:           return "zh-CN";
                default:                           return "en-US";
            }
        }
        if (project == "oakgame")
        {
            switch (language)
            {
                case ELanguage::English:            return "en";
                case ELanguage::French:             return "fr";
                case ELanguage::German:             return "de";
                case ELanguage::Italian:            return "it";
                case ELanguage::Spanish:            return "es";
                case ELanguage::Japanese:           return "ja";
                case ELanguage::Korean:             return "ko";
                case ELanguage::PortugueseBrazil:   return "pt-BR";
                case ELanguage::Russian:            return "ru";
                case ELanguage::Chinese:            return "zh-Hans-CN";
                case ELanguage::TraditionalChinese: return "zh-Hant-TW";
                default:                            return "en";
            }
        }
        if (project == "multiversus")
        {
            switch (language)
            {
                case ELanguage::English:          return "en";
                case ELanguage::French:           return "fr";
                case ELanguage::German:           return "de";
                case ELanguage::Italian:          return "it";
                case ELanguage::Spanish:          return "es";
                case ELanguage::SpanishLatin:     return "es-419";
                case ELanguage::Polish:           return "pl";
                case ELanguage::PortugueseBrazil: return "pt-BR";
                case ELanguage::Russian:          return "ru";
                case ELanguage::Chinese:          return "zh-Hans";
                default:                          return "en";
            }
        }
        if (project == "aion2")
        {
            switch (language)
            {
                case ELanguage::English:            return "en-US";
                case ELanguage::Korean:             return "ko-KR";
                case ELanguage::Japanese:           return "ja-JP";
                case ELanguage::TraditionalChinese: return "zh-TW";
                case ELanguage::Chinese:            return "zh-CN";
                case ELanguage::German:             return "de-DE";
                case ELanguage::French:             return "fr-FR";
                case ELanguage::Spanish:            return "es-ES";
                case ELanguage::PortugueseBrazil:   return "pt-BR";
                case ELanguage::Russian:            return "ru-RU";
                default:                            return "en-US";
            }
        }

        // https://www.alchemysoftware.com/livedocs/ezscript/Topics/Catalyst/Language.htm
        switch (language)
        {
            case ELanguage::English:            return "en";
            case ELanguage::AustralianEnglish:  return "en-AU";
            case ELanguage::BritishEnglish:     return "en-GB";
            case ELanguage::French:             return "fr";
            case ELanguage::German:             return "de";
            case ELanguage::Italian:            return "it";
            case ELanguage::Spanish:            return "es";
            case ELanguage::SpanishLatin:       return "es-419";
            case ELanguage::SpanishMexico:      return "es-MX";
            case ELanguage::Arabic:             return "ar";
            case ELanguage::Japanese:           return "ja";
            case ELanguage::Korean:             return "ko";
            case ELanguage::Polish:             return "pl";
            case ELanguage::Portuguese:         return "pt";
            case ELanguage::PortugueseBrazil:   return "pt-BR";
            case ELanguage::Russian:            return "ru";
            case ELanguage::Turkish:            return "tr";
            case ELanguage::Chinese:            return "zh";
            case ELanguage::TraditionalChinese: return "zh-Hant";
            case ELanguage::Swedish:            return "sv";
            case ELanguage::Thai:               return "th";
            case ELanguage::Indonesian:         return "id";
            case ELanguage::VietnameseVietnam:  return "vi-VN";
            case ELanguage::Zulu:               return "zu";
            default:                            return "en";
        }
    }

    int AbstractFileProvider::LoadVirtualPaths(const UE4::Versions::FPackageFileVersion& version)
    {
        (void)version; // Declared by C# and never read by its body — see the header.

        // C#'s regex, verbatim: the '.' before "upluginmanifest" is not escaped upstream, so it matches any
        // character there. Harmless in practice (the extension check has already narrowed the candidates)
        // and left alone.
        const std::regex manifestPattern("^" + EscapeRegex(ProjectName()) + "/Plugins/.+.upluginmanifest$",
                                         std::regex::icase);
        // C# also compiles an `arregex` for "<ProjectName>/Plugins/.*AssetRegistry.bin$" and never uses it;
        // with the misspelled extension above nothing reaches the arm it would have guarded, so it is not
        // reproduced here.

        VirtualPaths.clear();

        // C#'s Parallel.ForEach prefilter into a ConcurrentBag. Without a threading layer this is a plain
        // scan in Files' own enumeration order, which also makes the duplicate-key winner deterministic.
        std::vector<std::pair<std::string, std::shared_ptr<GameFile>>> matchingPlugins;
        Files.ForEach([&](const std::string& path, const std::shared_ptr<GameFile>& file)
        {
            for (const std::string& suffix : pluginExtensions)
            {
                if (EndsWithIgnoreCase(path, suffix))
                {
                    matchingPlugins.emplace_back(path, file);
                    break;
                }
            }
        });

        for (const auto& [filePath, gameFile] : matchingPlugins)
        {
            const std::string& extension = gameFile->Extension();
            if (extension == "upluginmanifest")
            {
                if (!std::regex_match(filePath, manifestPattern)) continue;
                const auto stream = TryCreateReader(gameFile->Path());
                if (stream == nullptr) continue;

                const auto json = Utils::Json::Parse(ReadAllText(*stream));
                if (!json.has_value()) continue;
                const auto manifest = UE4::Plugins::UPluginManifest::FromJson(*json);

                for (const auto& content : manifest.Contents)
                {
                    if (!content.Descriptor.CanContainContent) continue;

                    const auto virtPath = Utils::SubstringBeforeLast(Utils::SubstringAfterLast(content.File, '/'), '.');
                    const auto path = Utils::SubstringBeforeLast(RemoveAll(content.File, "../../../"), '/');
                    VirtualPaths[virtPath] = path;
                }
            }
            else if (extension == "uplugin")
            {
                if (VirtualPaths.count(gameFile->NameWithoutExtension()) != 0) continue;
                const auto stream = TryCreateReader(gameFile->Path());
                if (stream == nullptr) continue;

                const auto json = Utils::Json::Parse(ReadAllText(*stream));
                if (!json.has_value()) continue;
                const auto pluginFile = UE4::Plugins::UPluginDescriptor::FromJson(*json);
                if (!pluginFile.CanContainContent) continue;

                VirtualPaths[gameFile->NameWithoutExtension()] = gameFile->Directory();
            }
            else
            {
                // Unreachable — see pluginExtensions.
                VirtualPaths[Utils::SubstringAfterLast(gameFile->Directory(), '/')] = gameFile->Directory();
            }
        }

        return static_cast<int>(VirtualPaths.size());
    }

    bool AbstractFileProvider::LoadIniConfigs()
    {
        if (const auto defaultGame = TryGetGameFile("/Game/Config/DefaultGame.ini"))
        {
            if (const auto* vfsEntry = dynamic_cast<const UE4::VirtualFileSystem::VfsEntry*>(defaultGame.get()))
            {
                if (const auto* aesReader = dynamic_cast<const UE4::VirtualFileSystem::IAesVfsReader*>(vfsEntry->Vfs))
                    DefaultGame.EncryptionKeyGuid = aesReader->EncryptionKeyGuid();
            }
            if (const auto text = ReadIniText(*defaultGame))
                DefaultGame.Read(*text); // Read() clears the sections first, as C# does explicitly

            Internationalization.InitFromIni(DefaultGame);
        }

        if (const auto defaultEngine = TryGetGameFile("/Game/Config/DefaultEngine.ini"))
        {
            if (const auto* vfsEntry = dynamic_cast<const UE4::VirtualFileSystem::VfsEntry*>(defaultEngine.get()))
            {
                if (const auto* aesReader = dynamic_cast<const UE4::VirtualFileSystem::IAesVfsReader*>(vfsEntry->Vfs))
                    DefaultEngine.EncryptionKeyGuid = aesReader->EncryptionKeyGuid();
            }
            if (const auto text = ReadIniText(*defaultEngine))
                DefaultEngine.Read(*text);

            // C# also mirrors the a.StripAdditiveRefPose / r.*.KeepMobileMinLODSettingOnDesktop console
            // variables into Versions[<name>]; VersionContainer has no Options table here (see its header).

            for (const auto& section : DefaultEngine.Sections)
            {
                if (section->Name != "/Script/Engine.RendererSettings") continue;

                for (const auto& token : section->Tokens)
                {
                    const auto* instruction = dynamic_cast<const InstructionToken*>(token.get());
                    if (instruction == nullptr || instruction->Key != "r.DefaultFeature.LightUnits") continue;
                    try
                    {
                        size_t consumed = 0;
                        const int unit = std::stoi(instruction->Value, &consumed);
                        if (consumed > 0) DefaultLightUnit = static_cast<UE4::Objects::Engine::ELightUnits>(unit);
                    }
                    catch (const std::exception&)
                    {
                        // C#'s int.TryParse: a non-numeric value just leaves DefaultLightUnit alone.
                    }
                    break; // C#'s FirstOrDefault
                }
            }
        }

        return DefaultGame.FindSection("/Script/EngineSettings.GeneralProjectSettings") != nullptr;
    }

    IPackage& AbstractFileProvider::LoadPackage(GameFile& file)
    {
        if (!file.IsUePackage()) throw std::invalid_argument("cannot load non-UE package");

        // The provider owns loaded packages (see the OWNERSHIP note in the header); a second load of the
        // same file returns the cached one. C# constructs a fresh package each call and lets the GC own it.
        if (const auto cached = _loadedPackages.find(file.Path()); cached != _loadedPackages.end())
            return *cached->second.Package;

        std::shared_ptr<GameFile> uexp;
        std::vector<std::shared_ptr<GameFile>> ubulks, uptnls;
        Files.FindPayloads(file, uexp, ubulks, uptnls);

        // C#: `header => ubulks[0].SafeCreateReader(header)`. Only the first of each is used, as in C#.
        // The shared_ptr keeps the GameFile alive for as long as the package can still ask for the payload.
        UE4::Assets::Readers::FAssetArchive::RawPayloadProvider lazyUbulk, lazyUptnl;
        if (!ubulks.empty())
            lazyUbulk = [f = ubulks[0]](const UE4::Assets::Objects::FByteBulkDataHeader* header)
                { return f->SafeCreateReader(header); };
        if (!uptnls.empty())
            lazyUptnl = [f = uptnls[0]](const UE4::Assets::Objects::FByteBulkDataHeader* header)
                { return f->SafeCreateReader(header); };

        auto* ioStoreEntry = dynamic_cast<UE4::IO::Objects::FIoStoreEntry*>(&file);
        auto* vfsFileProvider = dynamic_cast<Vfs::IVfsFileProvider*>(this);

        if (ioStoreEntry == nullptr &&
            dynamic_cast<UE4::Pak::Objects::FPakEntry*>(&file) == nullptr &&
            dynamic_cast<Objects::OsGameFile*>(&file) == nullptr)
        {
            throw UE4::Exceptions::ParserException(
                "cannot load \"" + file.Path() + "\": unsupported game-file type");
        }

        LoadedPackage loaded;
        loaded.UassetAr = file.CreateReader();
        if (ioStoreEntry != nullptr && vfsFileProvider != nullptr)
        {
            // C#: new IoPackage(uasset, ioStoreEntry.IoStoreReader.ContainerHeader, lazyUbulk, lazyUptnl, vfsFileProvider)
            loaded.Package = std::make_unique<UE4::Assets::IoPackage>(
                *loaded.UassetAr, ioStoreEntry->GetIoStoreReader().ContainerHeader(),
                std::move(lazyUbulk), std::move(lazyUptnl), vfsFileProvider);
        }
        else
        {
            if (uexp != nullptr) loaded.UexpAr = uexp->CreateReader();
            loaded.Package = std::make_unique<Package>(*loaded.UassetAr, loaded.UexpAr.get(),
                                                      std::move(lazyUbulk), std::move(lazyUptnl), this);
        }

        auto [it, inserted] = _loadedPackages.emplace(file.Path(), std::move(loaded));
        return *it->second.Package;
    }

    IPackage* AbstractFileProvider::TryLoadPackage(const std::string& path)
    {
        const auto file = TryGetGameFile(path);
        return file != nullptr ? TryLoadPackage(*file) : nullptr;
    }

    IPackage* AbstractFileProvider::TryLoadPackage(GameFile& file)
    {
        try
        {
            return &LoadPackage(file);
        }
        catch (const std::exception&)
        {
            return nullptr;
        }
    }

    std::map<std::string, std::vector<uint8_t>> AbstractFileProvider::SavePackage(GameFile& file)
    {
        std::shared_ptr<GameFile> uexp;
        std::vector<std::shared_ptr<GameFile>> ubulks, uptnls;
        Files.FindPayloads(file, uexp, ubulks, uptnls, true);

        std::map<std::string, std::vector<uint8_t>> dict;
        dict[file.Path()] = file.Read();
        if (uexp != nullptr) dict[uexp->Path()] = uexp->Read();
        for (const auto& ubulk : ubulks) dict[ubulk->Path()] = ubulk->Read();
        for (const auto& uptnl : uptnls) dict[uptnl->Path()] = uptnl->Read();
        return dict;
    }

    void AbstractFileProvider::Dispose()
    {
        Files.Clear();
        VirtualPaths.clear();
        Internationalization.Clear();
        _loadedPackages.clear();
    }
}
