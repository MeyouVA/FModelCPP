// Ported from CUE4Parse/UE4/Wwise/Enums/EAkChannelConfigType.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums
{
    // C# tags this [JsonConverter(typeof(StringEnumConverter))] -- it serialises by member
    // name, not by number. The JSON writer is not ported yet; noted here so it is not lost.
    enum class EAkChannelConfigType : uint32_t
    {
        Anonymous            = 0x0,
        Standard             = 0x1,
        Ambisonic            = 0x2,
        Objects              = 0x3,
        Last                 = 0x4,
        UseDeviceMain        = 0xE,
        UseDevicePassthrough = 0xF,
    };
}
