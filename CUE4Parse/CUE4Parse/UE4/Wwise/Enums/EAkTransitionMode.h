// Ported from CUE4Parse/UE4/Wwise/Enums/EAkTransitionMode.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums
{
    enum class EAkTransitionMode : uint8_t
    {
        Disabled,
        CrossFadeAmp,
        CrossFadePower,
        Delay,
        SampleAccurate,
        TriggerRate,
    };
}
