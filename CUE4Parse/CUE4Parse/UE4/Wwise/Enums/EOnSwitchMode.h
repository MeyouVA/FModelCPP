// Ported from CUE4Parse/UE4/Wwise/Enums/EOnSwitchMode.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums
{
    enum class EOnSwitchMode : uint8_t
    {
        PlayToEnd = 0x0,
        Stop      = 0x1,
    };
}
