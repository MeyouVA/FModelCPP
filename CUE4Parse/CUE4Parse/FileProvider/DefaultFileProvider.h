// Ported from CUE4Parse/FileProvider/DefaultFileProvider.cs
// The concrete provider that scans a game directory: containers (.pak/.utoc) are registered for mounting,
// loose files with known UE extensions become OsGameFile entries.
//
// Deliberate differences from C#:
//   * System.IO.SearchOption becomes the SearchOption enum here; DirectoryInfo becomes
//     std::filesystem::path.
//   * The .uondemandtoc branch needs the on-demand downloader (unported); those files are skipped. TODO.
//   * RegisterTextureCache is a one-line map insert, done inline where C# calls the base helper.
//   * The obsolete isCaseInsensitive constructors are dropped; pass a StringComparer.
#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "Vfs/AbstractVfsFileProvider.h"

namespace CUE4Parse::FileProvider
{
    enum class SearchOption
    {
        TopDirectoryOnly,
        AllDirectories,
    };

    class DefaultFileProvider : public Vfs::AbstractVfsFileProvider
    {
    public:
        DefaultFileProvider(const std::filesystem::path& directory,
                            SearchOption searchOption,
                            UE4::Versions::VersionContainer versions = UE4::Versions::VersionContainer(),
                            Utils::StringComparer pathComparer = Utils::StringComparer::Ordinal());
        DefaultFileProvider(const std::filesystem::path& directory,
                            std::vector<std::filesystem::path> extraDirectories,
                            SearchOption searchOption,
                            UE4::Versions::VersionContainer versions = UE4::Versions::VersionContainer(),
                            Utils::StringComparer pathComparer = Utils::StringComparer::Ordinal());

        void Initialize() override;

    protected:
        std::filesystem::path _workingDirectory;
        std::vector<std::filesystem::path> _extraDirectories;
        SearchOption _searchOption;

    private:
        struct LooseFileDiscovery
        {
            UE4::VirtualFileSystem::GameFileMap Files;
            int FilesCount = 0;
        };

        LooseFileDiscovery IterateFiles(const std::filesystem::path& directory, SearchOption option);
    };
}
