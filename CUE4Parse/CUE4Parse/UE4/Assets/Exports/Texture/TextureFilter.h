// Ported from CUE4Parse/UE4/Assets/Exports/Texture/TextureFilter.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Assets::Exports::Texture
{
    enum class TextureFilter
    {
        TF_Nearest,
        TF_Bilinear,
        TF_Trilinear,
        // Use setting from the Texture Group.
        TF_Default,
        TF_MAX,
    };
}
