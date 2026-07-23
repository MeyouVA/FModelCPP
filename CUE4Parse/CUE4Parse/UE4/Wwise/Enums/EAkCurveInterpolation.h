// Ported from CUE4Parse/UE4/Wwise/Enums/EAkCurveInterpolation.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums
{
    // C# tags this [JsonConverter(typeof(StringEnumConverter))] -- it serialises by member
    // name, not by number. The JSON writer is not ported yet; noted here so it is not lost.
    enum class EAkCurveInterpolation : uint8_t
    {
        Log3          = 0x0,
        Sine          = 0x1,
        Log1          = 0x2,
        InvSCurve     = 0x3,
        Linear        = 0x4,
        SCurve        = 0x5,
        Exp1          = 0x6,
        SineRecip     = 0x7,
        Exp3          = 0x8,
        LastFadeCurve = 0x8,
        Constant      = 0x9,
    };
}
