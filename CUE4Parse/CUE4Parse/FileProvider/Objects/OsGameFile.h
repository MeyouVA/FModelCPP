// Ported from CUE4Parse/FileProvider/Objects/OsGameFile.cs
// A loose file on disk (the "no containers, just a cooked directory" case).
//
// Deliberate differences from C#:
//   * FileInfo becomes std::filesystem::path (kept as ActualFile, same name).
//   * The FByteBulkDataHeader partial-read branch is dropped with the type (see GameFile.h); Read always
//     returns the whole file. TODO with the bulk-data layer.
//   * C#'s baseDir constructor takes a mountPoint parameter it never uses; kept verbatim (the relative
//     path IS the mount-relative path).
#pragma once

#include <filesystem>
#include <string>

#include "VersionedGameFile.h"

namespace CUE4Parse::FileProvider::Objects
{
    class OsGameFile : public VersionedGameFile
    {
    public:
        std::filesystem::path ActualFile;

        OsGameFile(const std::filesystem::path& info, UE4::Versions::VersionContainer versions);
        OsGameFile(const std::filesystem::path& baseDir, const std::filesystem::path& info,
                   const std::string& mountPoint, UE4::Versions::VersionContainer versions);

        bool IsEncrypted() const override { return false; }
        Compression::CompressionMethod GetCompressionMethod() const override
        {
            return Compression::CompressionMethod::None;
        }

        std::vector<uint8_t> Read(const FByteBulkDataHeader* header = nullptr) override;
    };
}
