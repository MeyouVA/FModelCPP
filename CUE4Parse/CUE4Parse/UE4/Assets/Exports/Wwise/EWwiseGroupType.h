// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/EWwiseGroupType.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    enum class EWwiseGroupType : uint8_t
    {
        Switch  = 0,
        State   = 1,
        Unknown = 255,
    };
}
