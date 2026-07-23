// Ported from CUE4Parse/UE4/FMod/Enums/EAutomationConflictResolutionMethod.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::FMod::Enums
{
    enum class EAutomationConflictResolutionMethod : uint32_t
    {
        Resolution_LeastValue    = 0x0,
        Resolution_GreatestValue = 0x1,
        Resolution_Additive      = 0x2,
        Resolution_Average       = 0x3,
        Resolution_Multiply      = 0x4,
        Resolution_Override      = 0x5,
        Resolution_Undefined     = 0x6,
        Resolution_Max           = 0x7,
    };
}
