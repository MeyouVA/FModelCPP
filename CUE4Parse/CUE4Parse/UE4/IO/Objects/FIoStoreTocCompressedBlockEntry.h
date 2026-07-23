// Ported from CUE4Parse/UE4/IO/Objects/FIoStoreTocCompressedBlockEntry.cs
// 12 bytes: 40-bit offset + 24-bit compressed size packed in a ulong, then 24-bit uncompressed size +
// 8-bit compression method index packed in a uint.
#pragma once

#include <cstdint>
#include <cstdio>
#include <string>

#include "../../Readers/FArchive.h"

namespace CUE4Parse::UE4::IO::Objects
{
    struct FIoStoreTocCompressedBlockEntry
    {
        static constexpr int SIZE = 5 + 3 + 3 + 1;

        uint64_t Offset_CompressedSize = 0;
        uint32_t UncompressedSize_CompressionMethodIndex = 0;

        FIoStoreTocCompressedBlockEntry() = default;
        explicit FIoStoreTocCompressedBlockEntry(Readers::FArchive& Ar)
        {
            Offset_CompressedSize = Ar.Read<uint64_t>();
            UncompressedSize_CompressionMethodIndex = Ar.Read<uint32_t>();
        }

        static constexpr int OffsetBits = 40;
        static constexpr uint64_t OffsetMask = (1ULL << OffsetBits) - 1ULL;
        static constexpr int SizeBits = 24;
        static constexpr uint32_t SizeMask = (1u << SizeBits) - 1u;

        int64_t Offset() const { return static_cast<int64_t>(Offset_CompressedSize & OffsetMask); }
        uint32_t CompressedSize() const { return static_cast<uint32_t>((Offset_CompressedSize >> OffsetBits) & SizeMask); }
        uint32_t UncompressedSize() const { return UncompressedSize_CompressionMethodIndex & SizeMask; }
        uint8_t CompressionMethodIndex() const { return static_cast<uint8_t>(UncompressedSize_CompressionMethodIndex >> SizeBits); }

        std::string ToString() const
        {
            char buf[80];
            std::snprintf(buf, sizeof(buf), "Offset %lld: From %u To %u",
                          static_cast<long long>(Offset()), CompressedSize(), UncompressedSize());
            return buf;
        }
    };
}
