// Ported from CUE4Parse/UE4/Wwise/Enums/EAkClipAutomationType.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums
{
    enum class EAkClipAutomationType : uint32_t
    {
        Volume  = 0x0,
        LPF     = 0x1,
        HPF     = 0x2,
        FadeIn  = 0x3,
        FadeOut = 0x4,
    };
}
