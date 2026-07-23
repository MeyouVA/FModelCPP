#include "DefaultFileProvider.h"

#include <memory>
#include <stdexcept>
#include <utility>

#include "Objects/OsGameFile.h"
#include "../UE4/Versions/EGame.h"
#include "../Utils/StringUtils.h"

namespace CUE4Parse::FileProvider
{
    namespace fs = std::filesystem;
    using Objects::GameFile;
    using Objects::OsGameFile;
    using UE4::Versions::GAME_LordOfMysteries;

    namespace
    {
        std::string NormalizeSlashes(std::string s)
        {
            for (auto& c : s) if (c == '\\') c = '/';
            return s;
        }

        std::string Lower(std::string s)
        {
            for (auto& c : s) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c + 32);
            return s;
        }

        std::string Upper(std::string s)
        {
            for (auto& c : s) if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 32);
            return s;
        }

        bool ContainsIgnoreCase(const std::string& s, const std::string& value)
        {
            return Lower(s).find(Lower(value)) != std::string::npos;
        }

        // C# checks FullName against backslash-separated fragments (Windows paths); the port normalises the
        // path to forward slashes first, so the same containers are excluded on every platform.
        bool IsExcludedContainerPath(const std::string& fullName)
        {
            return ContainsIgnoreCase(fullName, "Binaries/ThirdParty/CEF") ||
                   Utils::Contains(fullName, "Binaries/Win32/host") ||
                   Utils::Contains(fullName, "Binaries/Win64/host") ||
                   Utils::Contains(fullName, "/qtwebengine_") ||
                   Utils::Contains(fullName, "NexonPlatformWebView/ThirdParty") ||
                   Utils::Contains(fullName, "SnapversePCGameSDK");
        }
    }

    DefaultFileProvider::DefaultFileProvider(const fs::path& directory, SearchOption searchOption,
                                             UE4::Versions::VersionContainer versions,
                                             Utils::StringComparer pathComparer)
        : DefaultFileProvider(directory, {}, searchOption, std::move(versions), pathComparer) {}

    DefaultFileProvider::DefaultFileProvider(const fs::path& directory, std::vector<fs::path> extraDirectories,
                                             SearchOption searchOption,
                                             UE4::Versions::VersionContainer versions,
                                             Utils::StringComparer pathComparer)
        : AbstractVfsFileProvider(std::move(versions), pathComparer),
          _workingDirectory(directory), _extraDirectories(std::move(extraDirectories)), _searchOption(searchOption)
    {
    }

    void DefaultFileProvider::Initialize()
    {
        if (!fs::exists(_workingDirectory))
            throw std::runtime_error("The game directory could not be found.");

        std::vector<LooseFileDiscovery> availableFiles;
        availableFiles.push_back(IterateFiles(_workingDirectory, _searchOption));
        for (const auto& directory : _extraDirectories)
            availableFiles.push_back(IterateFiles(directory, _searchOption));

        for (auto& osFiles : availableFiles)
        {
            Files.AddFiles(std::move(osFiles.Files));
            LooseFileCount += osFiles.FilesCount;
        }
    }

    DefaultFileProvider::LooseFileDiscovery DefaultFileProvider::IterateFiles(const fs::path& directory,
                                                                              SearchOption option)
    {
        LooseFileDiscovery result{UE4::VirtualFileSystem::GameFileMap{PathComparer}, 0};
        if (!fs::exists(directory)) return result;

        // Look for .uproject file to get the correct mount point
        fs::path uproject;
        bool hasUproject = false;
        for (const auto& entry : fs::directory_iterator(directory))
        {
            if (entry.is_regular_file() && Lower(entry.path().extension().string()) == ".uproject")
            {
                uproject = entry.path();
                hasUproject = true;
                break;
            }
        }

        std::string mountPoint;
        if (hasUproject)
        {
            mountPoint = Utils::SubstringBeforeLast(uproject.filename().string(), '.') + '/';
        }
        else
        {
            // Or use the directory name
            mountPoint = directory.filename().string() + '/';
        }

        // In .uproject mode, we must recursively look for files
        option = hasUproject ? SearchOption::AllDirectories : option;

        const auto handleFile = [&](const fs::path& file)
        {
            const std::string upperExt = Upper(Utils::SubstringAfter(file.extension().string(), '.'));
            const std::string fullName = NormalizeSlashes(file.string());

            // Only load containers if .uproject file is not found
            if (!hasUproject && (upperExt == "PAK" || upperExt == "UTOC" ||
                                 (upperExt == "UPAK" && Versions.Game() == GAME_LordOfMysteries)))
            {
                if (IsExcludedContainerPath(fullName)) return;
                RegisterVfs(fullName);
                return;
            }

            // C#'s UONDEMANDTOC branch needs the on-demand downloader (unported). TODO.

            if (upperExt == "TFC")
            {
                // C#'s RegisterTextureCache(file)
                TextureCachePaths[Utils::SubstringBeforeLast(file.filename().string(), '.')] = fullName;
                return;
            }

            // Register local file only if it has a known extension, we don't need every file
            const std::string lowerExt = Lower(upperExt);
            if (GameFile::UeKnownExtensionsSet().find(lowerExt) == GameFile::UeKnownExtensionsSet().end())
                return;
            if (GameFile::UePackagePayloadExtensionsSet().find(lowerExt) == GameFile::UePackagePayloadExtensionsSet().end())
                ++result.FilesCount;

            // C# quirk kept verbatim: the base directory is ALWAYS _workingDirectory, even when iterating an
            // extra directory, so extra-directory paths come out relative to the working directory.
            auto osFile = std::make_shared<OsGameFile>(_workingDirectory, file, mountPoint, Versions);
            result.Files[osFile->Path()] = std::move(osFile);
        };

        if (option == SearchOption::AllDirectories)
        {
            for (const auto& entry : fs::recursive_directory_iterator(directory))
                if (entry.is_regular_file()) handleFile(entry.path());
        }
        else
        {
            for (const auto& entry : fs::directory_iterator(directory))
                if (entry.is_regular_file()) handleFile(entry.path());
        }

        return result;
    }
}
