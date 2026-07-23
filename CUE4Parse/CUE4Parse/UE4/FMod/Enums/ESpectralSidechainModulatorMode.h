// Ported from CUE4Parse/UE4/FMod/Enums/ESpectralSidechainModulatorMode.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::FMod::Enums
{
    enum class ESpectralSidechainModulatorMode
    {
        RMS              = 0,
        SpectralCentroid = 1,
        Max              = 2,
    };
}
