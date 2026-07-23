// Ported from CUE4Parse/UE4/Wwise/Enums/EAkPluginType.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums
{
    // C# tags this [JsonConverter(typeof(StringEnumConverter))] -- it serialises by member
    // name, not by number. The JSON writer is not ported yet; noted here so it is not lost.
    enum class EAkPluginType : int32_t
    {
        None            = 0x0,
        Codec           = 0x1,
        Source          = 0x2,
        Effect          = 0x3,
        MotionDevice    = 0x4, // 125 <=
        MotionSource    = 0x5, // 125 <=
        Mixer           = 0x6,
        Sink            = 0x7,
        GlobalExtension = 0x8,
        Metadata        = 0x9,
        Last            = 0xA,
        Mask            = 0xF,
    };
}
