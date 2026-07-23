// Ported from CUE4Parse/UE4/Assets/Exports/Texture/ETextureCookPlatformTilingSettings.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Assets::Exports::Texture
{
    enum class ETextureCookPlatformTilingSettings
    {
        TCPTS_FromTextureGroup,
        TCPTS_Tile,
        TCPTS_DoNotTile,
    };
}
