// Ported from CUE4Parse/UE4/Wwise/Enums/EAkSyncType.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums
{
    // C# tags this [JsonConverter(typeof(StringEnumConverter))] -- it serialises by member
    // name, not by number. The JSON writer is not ported yet; noted here so it is not lost.
    enum class EAkSyncType : uint32_t
    {
        Immediate        = 0x0,
        NextGrid         = 0x1,
        NextBar          = 0x2,
        NextBeat         = 0x3,
        NextMarker       = 0x4,
        NextUserMarker   = 0x5,
        EntryMarker      = 0x6,
        ExitMarker       = 0x7,
        ExitNever        = 0x8,
        LastExitPosition = 0x9,
    };
}
