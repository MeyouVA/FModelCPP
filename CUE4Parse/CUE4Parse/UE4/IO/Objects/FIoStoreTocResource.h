// Ported from CUE4Parse/UE4/IO/Objects/FIoStoreTocResource.cs
// The parsed .utoc: header, chunk ids + offsets, perfect-hash tables, compression blocks and methods, and
// the (lazily fetched) directory-index buffer.
//
// Deliberate differences from C#:
//   * The TheFinals/ArcRaiders and NeedForSpeedMobile branches ARE ported — their obfuscation is
//     self-contained AES with hardcoded keys, not GameTypes code.
//   * C# holds the toc archive as `FArchive? _tocAr`; here it is a shared_ptr handed in by the reader.
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "FIoChunkId.h"
#include "FIoOffsetAndLength.h"
#include "FIoStoreTocCompressedBlockEntry.h"
#include "FIoStoreTocEntryMeta.h"
#include "FIoStoreTocHeader.h"
#include "../../Readers/FArchive.h"
#include "../../../Compression/CompressionMethod.h"

namespace CUE4Parse::UE4::IO::Objects
{
    enum class EIoStoreTocReadOptions
    {
        Default = 0,
        ReadDirectoryIndex = 1 << 0,
        ReadTocMeta = 1 << 1,
        ReadAll = ReadDirectoryIndex | ReadTocMeta,
    };

    inline bool HasFlag(EIoStoreTocReadOptions value, EIoStoreTocReadOptions flag)
    {
        return (static_cast<int>(value) & static_cast<int>(flag)) != 0;
    }

    class FIoStoreTocResource
    {
    public:
        std::unique_ptr<FIoStoreTocHeader> Header; // pointer only because the header has no default ctor
        std::vector<FIoChunkId> ChunkIds;
        std::vector<FIoOffsetAndLength> ChunkOffsetLengths;
        std::optional<std::vector<int32_t>> ChunkPerfectHashSeeds;
        std::optional<std::vector<int32_t>> ChunkIndicesWithoutPerfectHash;
        std::vector<FIoStoreTocCompressedBlockEntry> CompressionBlocks;
        std::vector<Compression::CompressionMethod> CompressionMethods;

        int64_t DirectoryIndexBufferOffset = -1;
        std::optional<std::vector<FIoStoreTocEntryMeta>> ChunkMetas;

        FIoStoreTocResource(std::shared_ptr<Readers::FArchive> Ar,
                            EIoStoreTocReadOptions readOptions = EIoStoreTocReadOptions::Default);

        // The raw (possibly still encrypted) directory-index bytes, or nullopt when there is no index.
        std::optional<std::vector<uint8_t>> GetDirectoryIndexBuffer() const;

    private:
        std::shared_ptr<Readers::FArchive> _tocAr;
    };
}
