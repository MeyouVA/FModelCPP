// Ported from CUE4Parse/UE4/Wwise/Enums/EAkMusicTrackType.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums
{
    enum class EAkMusicTrackType : uint8_t
    {
        Normal,
        Random,
        Sequence,
        Switch,
    };
}
