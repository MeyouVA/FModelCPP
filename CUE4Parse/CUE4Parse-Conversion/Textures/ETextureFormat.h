// Ported from CUE4Parse-Conversion/Textures/ETextureFormat.cs
#pragma once

namespace CUE4Parse_Conversion::Textures
{
    enum class ETextureFormat
    {
        Png,
        Jpeg,
        Tga,
        Dds
    };

    inline const char* Description(ETextureFormat value)
    {
        switch (value)
        {
            case ETextureFormat::Png:  return "PNG";
            case ETextureFormat::Jpeg: return "JPEG";
            case ETextureFormat::Tga:  return "TGA";
            case ETextureFormat::Dds:  return "DDS (Not Implemented)";
        }
        return "";
    }
}
