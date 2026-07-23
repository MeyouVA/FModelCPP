// Ported from CUE4Parse/UE4/FMod/Enums/EClockSource.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::FMod::Enums
{
    enum class EClockSource : uint32_t
    {
        ClockSource_Local  = 0x0,
        ClockSource_Global = 0x1,
        ClockSource_Max    = 0x2,
    };
}
