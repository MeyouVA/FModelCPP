// Ported from CUE4Parse/UE4/Wwise/Enums/EAkTransitionRampingType.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums
{
    enum class EAkTransitionRampingType : uint32_t
    {
        None              = 0x0,
        SlewRate          = 0x1,
        FilteringOverTime = 0x2,
    };
}
