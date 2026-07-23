// Ported from CUE4Parse/UE4/Assets/Exports/Material/ETextureChannel.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Assets::Exports::Material
{
    enum class ETextureChannel
    {
        TC_NONE,
        TC_R,
        TC_G,
        TC_B,
        TC_A,
        TC_MA,
    };
}
