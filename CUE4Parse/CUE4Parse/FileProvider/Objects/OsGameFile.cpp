#include "OsGameFile.h"

#include <fstream>
#include <stdexcept>

namespace CUE4Parse::FileProvider::Objects
{
    namespace
    {
        std::string NormalizeSlashes(std::string s)
        {
            for (auto& c : s) if (c == '\\') c = '/';
            return s;
        }

        int64_t FileSizeOrZero(const std::filesystem::path& p)
        {
            std::error_code ec;
            const auto size = std::filesystem::file_size(p, ec);
            return ec ? 0 : static_cast<int64_t>(size);
        }
    }

    OsGameFile::OsGameFile(const std::filesystem::path& info, UE4::Versions::VersionContainer versions)
        : VersionedGameFile(NormalizeSlashes(info.string()), FileSizeOrZero(info), std::move(versions)),
          ActualFile(info) {}

    OsGameFile::OsGameFile(const std::filesystem::path& baseDir, const std::filesystem::path& info,
                           const std::string& mountPoint, UE4::Versions::VersionContainer versions)
        : VersionedGameFile(NormalizeSlashes(std::filesystem::relative(info, baseDir).string()),
                            FileSizeOrZero(info), std::move(versions)),
          ActualFile(info)
    {
        (void) mountPoint; // unused in C# too — see header comment
    }

    std::vector<uint8_t> OsGameFile::Read()
    {
        std::ifstream stream(ActualFile, std::ios::binary);
        if (!stream) throw std::runtime_error("failed to open file: " + ActualFile.string());

        std::vector<uint8_t> bytes(static_cast<size_t>(FileSizeOrZero(ActualFile)));
        stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (stream.gcount() != static_cast<std::streamsize>(bytes.size()))
            throw std::runtime_error("failed to read whole file: " + ActualFile.string());
        return bytes;
    }
}
