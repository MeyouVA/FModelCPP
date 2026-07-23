// Ported from CUE4Parse/MappingsProvider/Usmap/EUsmapCompressionMethod.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::MappingsProvider::Usmap
{
    enum class EUsmapCompressionMethod : uint8_t
    {
        None,
        Oodle,
        Brotli,
        ZStandard,
        Unknown   = 0xFF,
    };
}
