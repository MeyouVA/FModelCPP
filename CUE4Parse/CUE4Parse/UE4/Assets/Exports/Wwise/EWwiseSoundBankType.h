// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/EWwiseSoundBankType.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    enum class EWwiseSoundBankType : uint8_t
    {
        User                    = 0,
        Event                   = 30,
        Bus                     = 31,
        EWwiseSoundBankType_MAX = 32,
    };
}
