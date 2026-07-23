// Ported from CUE4Parse/UE4/Wwise/Enums/EAkRtpcAccum.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums
{
    enum class EAkRtpcAccum : uint8_t
    {
        None,
        Exclusive,
        Additive,
        Multiply,
        Boolean,
        Maximum,
        Filter,
    };
}
