// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/EWwiseLanguageRequirement.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    enum class EWwiseLanguageRequirement : uint8_t
    {
        IsDefault                     = 0,
        IsOptional                    = 1,
        SFX                           = 2,
        EWwiseLanguageRequirement_MAX = 3,
    };
}
