// Ported from CUE4Parse/Compression/LZ4.cs.
//
// C# P/Invokes into a native liblz4 (LZ4_decompress_safe). The block format is small and standard, so the
// C++ port implements the safe block decoder inline instead of depending on an external library. This is the
// LZ4 *block* format (as UE stores it), not the LZ4 frame format.
#pragma once

#include <cstdint>

namespace CUE4Parse::Compression
{
    // Decompress an LZ4 block. Returns the number of bytes written to dst, or a negative value on error
    // (bad input, or output would exceed dstCapacity). Mirrors liblz4's LZ4_decompress_safe.
    int LZ4_decompress_safe(const uint8_t* src, uint8_t* dst, int compressedSize, int dstCapacity);
}
