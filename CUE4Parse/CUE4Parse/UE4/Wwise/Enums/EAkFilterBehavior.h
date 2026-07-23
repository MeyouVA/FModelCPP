// Ported from CUE4Parse/UE4/Wwise/Enums/EAkFilterBehavior.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums
{
    // AkFilterBehavior::FilterBehavior
    enum class EAkFilterBehavior : uint16_t
    {
        Additive = 0x0,
        Maximum  = 0x1,
    };
}
