// Ported from CUE4Parse/UE4/Wwise/Enums/EAkBelowThresholdBehavior.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums
{
    enum class EAkBelowThresholdBehavior : uint8_t
    {
        ContinueToPlay           = 0x0,
        KillVoice                = 0x1,
        SetAsVirtualVoice        = 0x2,
        KillIfOneShotElseVirtual = 0x3,
    };
}
