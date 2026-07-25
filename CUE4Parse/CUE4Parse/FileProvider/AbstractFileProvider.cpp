#include "AbstractFileProvider.h"

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
#include "../UE4/Readers/FArchive.h"
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
    }

    AbstractFileProvider::AbstractFileProvider(UE4::Versions::VersionContainer versions,
                                               Utils::StringComparer pathComparer)
        : Versions(std::move(versions)), PathComparer(pathComparer),
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
