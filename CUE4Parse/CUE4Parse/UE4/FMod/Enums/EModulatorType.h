// Ported from CUE4Parse/UE4/FMod/Enums/EModulatorType.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::FMod::Enums
{
    enum class EModulatorType : int32_t
    {
        ADSR              = 0,
        Random            = 1,
        Envelope          = 2,
        LFO               = 3,
        Seek              = 4,
        SpectralSidechain = 5,
    };
}
