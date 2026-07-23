// Ported from CUE4Parse/UE4/Wwise/Enums/EAkGameSyncType.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums
{
    enum class EAkGameSyncType : uint8_t
    {
        GameParameter,
        MIDIParameter,
        Switch,
        State,
        Modulator,
        Count,
    };
}
