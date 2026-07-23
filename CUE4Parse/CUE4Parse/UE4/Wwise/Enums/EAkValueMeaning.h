// Ported from CUE4Parse/UE4/Wwise/Enums/EAkValueMeaning.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums
{
    enum class EAkValueMeaning
    {
        Default     = 0x0,
        Independent = 0x1,
        Offset      = 0x2,
    };
}
