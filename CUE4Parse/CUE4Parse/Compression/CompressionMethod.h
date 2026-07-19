// Ported from CUE4Parse/Compression/CompressionMethod.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::Compression
{
    // Values match the C# enum exactly (the numbering gap after Oodle=4 is implicit-sequential in C#).
    enum class CompressionMethod : int32_t
    {
        None = 0,
        Zlib = 1,
        Gzip = 2, //???
        Custom = 3,
        Oodle = 4,
        LZ4,
        LZO,
        Zstd,
        XB1Zlib,
        XboxOneGDKZlib,
        Brotli,
        PWC, // Century: Age of Ashes (custom obfuscation)
        Unknown
    };
}
