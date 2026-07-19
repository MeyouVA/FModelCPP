// Ported from CUE4Parse/UE4/Assets/Objects/FCompressedChunk.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Assets::Objects
{
    // Package-level compression chunk descriptor (4 ints; read as a POD via Read<FCompressedChunk>).
    struct FCompressedChunk
    {
        int32_t UncompressedOffset = 0;
        int32_t UncompressedSize = 0;
        int32_t CompressedOffset = 0;
        int32_t CompressedSize = 0;
    };
}
