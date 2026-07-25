// Ported from CUE4Parse/FileProvider/Objects/VersionedGameFile.cs
// A GameFile that carries its own VersionContainer so CreateReader can hand it to the archive.
#pragma once

#include <memory>
#include <string>
#include <utility>

#include "GameFile.h"
#include "../../UE4/Readers/FByteArchive.h"
#include "../../UE4/Versions/VersionContainer.h"

namespace CUE4Parse::FileProvider::Objects
{
    class VersionedGameFile : public GameFile
    {
    public:
        UE4::Versions::VersionContainer Versions;

        std::unique_ptr<UE4::Readers::FArchive> CreateReader(const FByteBulkDataHeader* header = nullptr) override
        {
            return std::make_unique<UE4::Readers::FByteArchive>(Path(), Read(header), Versions);
        }

    protected:
        VersionedGameFile(std::string path, int64_t size, UE4::Versions::VersionContainer versions)
            : GameFile(std::move(path), size), Versions(std::move(versions)) {}
    };
}
