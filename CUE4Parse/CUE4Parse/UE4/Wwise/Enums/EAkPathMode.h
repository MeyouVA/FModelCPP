// Ported from CUE4Parse/UE4/Wwise/Enums/EAkPathMode.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums
{
    enum class EAkPathMode : uint8_t
    {
        StepSequence       = 0x0,
        StepRandom         = 0x1,
        ContinuousSequence = 0x2,
        ContinuousRandom   = 0x3,
    };
}
