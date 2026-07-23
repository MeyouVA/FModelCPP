// Ported from CUE4Parse/UE4/Objects/Engine/ELightUnits.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Objects::Engine
{
    enum class ELightUnits : uint8_t
    {
        Unitless,
        Candelas,
        Lumens,
        EV,
        Nits
    };
}
