// Ported from CUE4Parse/UE4/Objects/Core/Compression/FCompressedBuffer.cs
// A header plus the still-compressed bytes behind it. Nothing here decompresses: C# only reads the payload
// into an array too, and the Oodle/LZ4 block walk that would follow lives in the conversion layer.
#pragma once

#include <cstdint>
#include <vector>

#include "FCompressedBufferHeader.h"
#include "../../../Readers/FArchive.h"

namespace CUE4Parse::UE4::Objects::Core::Compression
{
    class FCompressedBuffer
    {
    public:
        FCompressedBufferHeader Header;
        std::vector<uint8_t> Data;

        FCompressedBuffer() = default;

        explicit FCompressedBuffer(Readers::FArchive& Ar)
        {
            Header = FCompressedBufferHeader(Ar);

            constexpr uint64_t MaxCompressedSize = 1ull << 48;
            constexpr uint64_t headerSize = 64; // hardcode for now
            if (Header.Magic == FCompressedBufferHeader::ExpectedMagic &&
                Header.TotalCompressedSize >= headerSize &&
                Header.TotalCompressedSize <= MaxCompressedSize)
            {
                Data = Ar.ReadArray<uint8_t>(static_cast<int>(Header.TotalCompressedSize - headerSize));
            }
        }
    };
}
