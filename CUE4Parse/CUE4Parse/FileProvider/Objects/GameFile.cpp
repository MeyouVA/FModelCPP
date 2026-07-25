#include "GameFile.h"

#include <algorithm>

#include "../../UE4/Readers/FArchive.h"

namespace CUE4Parse::FileProvider::Objects
{
    const std::vector<std::string> GameFile::UePackageExtensions = {"uasset", "umap"};
    const std::vector<std::string> GameFile::UePackagePayloadExtensions = {"uexp", "ubulk", "uptnl"};
    const std::vector<std::string> GameFile::UeKnownExtensions = {
        "uasset", "umap",
        "uexp", "ubulk", "uptnl",
        "bin", "ini", "uplugin", "upluginmanifest", "locres", "locmeta",
        "wem", "bnk", "pck", "bank", "awb", "acb"
    };

    namespace
    {
        std::unordered_set<std::string> Lowered(const std::vector<std::string>& source)
        {
            std::unordered_set<std::string> set;
            for (const auto& s : source) set.insert(s); // the source lists are already lowercase
            return set;
        }
    }

    const std::unordered_set<std::string>& GameFile::UePackageExtensionsSet()
    {
        static const std::unordered_set<std::string> set = Lowered(UePackageExtensions);
        return set;
    }

    const std::unordered_set<std::string>& GameFile::UePackagePayloadExtensionsSet()
    {
        static const std::unordered_set<std::string> set = Lowered(UePackagePayloadExtensions);
        return set;
    }

    const std::unordered_set<std::string>& GameFile::UeKnownExtensionsSet()
    {
        static const std::unordered_set<std::string> set = Lowered(UeKnownExtensions);
        return set;
    }

    bool GameFile::IsKnown(const std::unordered_set<std::string>& set, const std::string& extension)
    {
        std::string lower = extension;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](char c) { return static_cast<char>(c >= 'A' && c <= 'Z' ? c + 32 : c); });
        return set.find(lower) != set.end();
    }

    std::optional<std::vector<uint8_t>> GameFile::SafeRead(const FByteBulkDataHeader* header)
    {
        try { return Read(header); }
        catch (const std::exception&) { return std::nullopt; }
    }

    std::unique_ptr<UE4::Readers::FArchive> GameFile::SafeCreateReader(const FByteBulkDataHeader* header)
    {
        try { return CreateReader(header); }
        catch (const std::exception&) { return nullptr; }
    }
}
