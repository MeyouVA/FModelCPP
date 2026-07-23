// Ported from CUE4Parse/UE4/Wwise/Enums/EAkBuiltInParam.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums
{
    enum class EAkBuiltInParam : uint8_t
    {
        None             = 0x0,
        Start            = 0x1,
        Distance         = 0x1,
        Azimuth          = 0x2,
        Elevation        = 0x3,
        EmitterCone      = 0x4,
        Obstruction      = 0x5,
        Occlusion        = 0x6,
        ListenerCone     = 0x7,
        Diffraction      = 0x8,
        TransmissionLoss = 0x9,
        Max              = 0xA,
    };
}
