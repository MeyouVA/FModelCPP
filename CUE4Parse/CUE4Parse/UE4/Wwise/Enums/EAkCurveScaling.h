// Ported from CUE4Parse/UE4/Wwise/Enums/EAkCurveScaling.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums
{
    enum class EAkCurveScaling : uint8_t
    {
        None,
        // Unsupported = 0x1
        dB      = 0x2,
        Log,
        dBToLin,
    };
}
