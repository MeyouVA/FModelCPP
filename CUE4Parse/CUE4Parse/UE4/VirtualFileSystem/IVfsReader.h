// Ported from CUE4Parse/UE4/VirtualFileSystem/IVfsReader.cs
// The interface every mountable container (pak, utoc, ...) implements.
//
// Deliberate differences from C#:
//   * IDisposable becomes the virtual destructor.
//   * `IReadOnlyDictionary<string, GameFile> Files` becomes an ordered std::map keyed with the
//     StringComparer the caller passed to Mount, holding shared_ptr because the updated pak index can
//     alias one non-encoded entry under several paths.
//   * MountTo/FileProviderDictionary are not ported: FileProvider/Vfs has no C++ counterpart yet. TODO.
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../../FileProvider/Objects/GameFile.h"
#include "../../Utils/StringComparer.h"
#include "../Versions/VersionContainer.h"

namespace CUE4Parse::UE4::Assets::Objects { struct FByteBulkDataHeader; }

namespace CUE4Parse::UE4::VirtualFileSystem
{
    class VfsEntry;

    using GameFileMap = std::map<std::string, std::shared_ptr<FileProvider::Objects::GameFile>, Utils::StringComparer>;

    class IVfsReader
    {
    public:
        virtual ~IVfsReader() = default;

        virtual const std::string& Path() const = 0;
        virtual const std::string& Name() const = 0;
        virtual int64_t ReadOrder() const = 0;

        virtual const GameFileMap& Files() const = 0;
        int FileCount() const { return static_cast<int>(Files().size()); }

        virtual const std::string& MountPoint() const = 0;
        virtual bool HasDirectoryIndex() const = 0;

        virtual bool IsConcurrent() const = 0;
        virtual void SetConcurrent(bool value) = 0;

        virtual Versions::VersionContainer& GetVersions() = 0;
        virtual Versions::EGame Game() const = 0;
        virtual Versions::FPackageFileVersion Ver() const = 0;

        virtual void Mount(const Utils::StringComparer& pathComparer) = 0;
        virtual std::vector<uint8_t> Extract(VfsEntry& entry, const Assets::Objects::FByteBulkDataHeader* header = nullptr) = 0;
    };
}
