// Ported from CUE4Parse/MappingsProvider/Usmap/EUsmapVersion.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::MappingsProvider::Usmap
{
    enum class EUsmapVersion : uint8_t
    {
        // Initial format.
        Initial,
        // Adds package versioning to aid with compatibility
        PackageVersioning,
        // Adds support for 16-bit wide name-lengths (ushort/uint16)
        LongFName,
        // Adds support for enums with more than 255 values
        LargeEnums,
        // Adds support for explicit enum values
        ExplicitEnumValues,
        LatestPlusOne,
        Latest             = LatestPlusOne - 1,
    };
}
