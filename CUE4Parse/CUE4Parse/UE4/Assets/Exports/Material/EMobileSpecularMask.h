// Ported from CUE4Parse/UE4/Assets/Exports/Material/EMobileSpecularMask.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Assets::Exports::Material
{
    enum class EMobileSpecularMask
    {
        MSM_Constant,
        MSM_Luminance,
        MSM_DiffuseRed,
        MSM_DiffuseGreen,
        MSM_DiffuseBlue,
        MSM_DiffuseAlpha,
        MSM_MaskTextureRGB,
        MSM_MaskTextureRed,
        MSM_MaskTextureGreen,
        MSM_MaskTextureBlue,
        MSM_MaskTextureAlpha,
    };
}
