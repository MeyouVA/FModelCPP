#include "OsGameFile.h"

#include "../../UE4/Assets/Objects/FByteBulkDataHeader.h"

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

    std::vector<uint8_t> OsGameFile::Read(const FByteBulkDataHeader* header)
    {
        std::ifstream stream(ActualFile, std::ios::binary);
        if (!stream) throw std::runtime_error("failed to open file: " + ActualFile.string());

        // A header asks for one sub-range of the file rather than the whole of it.
        if (header != nullptr)
        {
            stream.seekg(static_cast<std::streamoff>(header->OffsetInFile), std::ios::beg);
            std::vector<uint8_t> partial(static_cast<size_t>(header->SizeOnDisk));
            stream.read(reinterpret_cast<char*>(partial.data()), static_cast<std::streamsize>(partial.size()));
            if (stream.gcount() != static_cast<std::streamsize>(partial.size()))
                throw std::runtime_error("failed to read requested bulk range: " + ActualFile.string());
            return partial;
        }

        std::vector<uint8_t> bytes(static_cast<size_t>(FileSizeOrZero(ActualFile)));
        stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (stream.gcount() != static_cast<std::streamsize>(bytes.size()))
            throw std::runtime_error("failed to read whole file: " + ActualFile.string());
        return bytes;
    }
}
