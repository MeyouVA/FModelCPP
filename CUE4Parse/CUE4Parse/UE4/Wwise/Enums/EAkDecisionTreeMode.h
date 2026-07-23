// Ported from CUE4Parse/UE4/Wwise/Enums/EAkDecisionTreeMode.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums
{
    enum class EAkDecisionTreeMode : uint8_t
    {
        Mode_BestMatch = 0x0,
        Mode_Weighted  = 0x1,
    };
}
