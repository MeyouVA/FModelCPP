// Ported from CUE4Parse/UE4/IO/Objects/FIoStoreTocHeader.cs
// The fixed 144-byte .utoc header (magic "-==--==--==--==-").
#pragma once

#include <array>
#include <cstdint>

#include "FIoContainerId.h"
#include "../../Objects/Core/Misc/FGuid.h"
#include "../../Readers/FArchive.h"

namespace CUE4Parse::UE4::IO::Objects
{
    enum class EIoStoreTocVersion : uint8_t
    {
        Invalid = 0,
        Initial,
        DirectoryIndex,
        PartitionSize,
        PerfectHash,
        PerfectHashWithOverflow,
        OnDemandMetaData,
        RemovedOnDemandMetaData,
        ReplaceIoChunkHashWithIoHash,
        LatestPlusOne,
        Latest = LatestPlusOne - 1,
    };

    enum class EIoContainerFlags : uint32_t
    {
        None = 0,
        Compressed = 1 << 0,
        Encrypted = 1 << 1,
        Signed = 1 << 2,
        Indexed = 1 << 3,
        OnDemand = 1 << 4,
    };

    inline bool HasFlag(EIoContainerFlags value, EIoContainerFlags flag)
    {
        return (static_cast<uint32_t>(value) & static_cast<uint32_t>(flag)) != 0;
    }

    class FIoStoreTocHeader
    {
    public:
        static constexpr int SIZE = 144;
        static const std::array<uint8_t, 16> TOC_MAGIC; // -==--==--==--==-

        std::array<uint8_t, 16> TocMagic{};
        EIoStoreTocVersion Version = EIoStoreTocVersion::Invalid;
        uint32_t TocHeaderSize = 0;
        uint32_t TocEntryCount = 0;
        uint32_t TocCompressedBlockEntryCount = 0;
        uint32_t TocCompressedBlockEntrySize = 0; // For sanity checking
        uint32_t CompressionMethodNameCount = 0;
        uint32_t CompressionMethodNameLength = 0;
        uint32_t CompressionBlockSize = 0;
        uint32_t DirectoryIndexSize = 0;
        uint32_t PartitionCount = 0;
        FIoContainerId ContainerId;
        UE4::Objects::Core::Misc::FGuid EncryptionKeyGuid;
        EIoContainerFlags ContainerFlags = EIoContainerFlags::None;
        uint32_t TocChunkPerfectHashSeedsCount = 0;
        uint64_t PartitionSize = 0;
        uint32_t TocChunksWithoutPerfectHashCount = 0;

        explicit FIoStoreTocHeader(Readers::FArchive& Ar);
    };
}
