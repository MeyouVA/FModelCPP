// Ported from CUE4Parse/UE4/VirtualFileSystem/VfsEntry.cs
// A GameFile that lives at some offset inside a mounted container.
#pragma once

#include <cstdint>
#include <string>
#include <utility>

#include "IVfsReader.h"
#include "../../FileProvider/Objects/GameFile.h"

namespace CUE4Parse::UE4::VirtualFileSystem
{
    class VfsEntry : public FileProvider::Objects::GameFile
    {
    public:
        // C#'s `IVfsReader Vfs { get; }`. Non-owning: the reader owns its entries, not the other way round.
        IVfsReader* Vfs = nullptr;
        int64_t Offset = 0;

    protected:
        VfsEntry(IVfsReader* vfs, std::string path, int64_t size = 0)
            : GameFile(std::move(path), size), Vfs(vfs) {}

        explicit VfsEntry(IVfsReader* vfs) : Vfs(vfs) {}
    };
}
