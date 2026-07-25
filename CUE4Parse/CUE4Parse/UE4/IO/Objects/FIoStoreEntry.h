// Ported from CUE4Parse/UE4/IO/Objects/FIoStoreEntry.cs
// A file inside a mounted IO Store container, addressed by its toc entry index.
#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "FIoChunkId.h"
#include "../../VirtualFileSystem/VfsEntry.h"

namespace CUE4Parse::UE4::IO { class IoStoreReader; }

namespace CUE4Parse::UE4::IO::Objects
{
    class FIoStoreEntry : public VirtualFileSystem::VfsEntry
    {
    public:
        // The indexed constructor (path from the directory index).
        FIoStoreEntry(IoStoreReader* reader, std::string path, uint32_t tocEntryIndex);
        // The non-indexed constructor: the path is synthesised as "NonIndexed/0x{ChunkId}.{ext}".
        FIoStoreEntry(IoStoreReader* reader, uint32_t tocEntryIndex);

        bool IsEncrypted() const override;
        Compression::CompressionMethod GetCompressionMethod() const override;

        const FIoChunkId& ChunkId() const;
        IoStoreReader& GetIoStoreReader() const;

        std::vector<uint8_t> Read(const CUE4Parse::UE4::Assets::Objects::FByteBulkDataHeader* header = nullptr) override;
        std::unique_ptr<Readers::FArchive> CreateReader(const CUE4Parse::UE4::Assets::Objects::FByteBulkDataHeader* header = nullptr) override;

    private:
        uint32_t _tocEntryIndex = 0;
    };
}
