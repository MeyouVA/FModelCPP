// Ported from CUE4Parse/UE4/Wwise/Enums/EAkRandomMode.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums
{
    enum class EAkRandomMode : uint8_t
    {
        Normal,
        Shuffle,
    };
}
