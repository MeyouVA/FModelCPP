// Ported from CUE4Parse/UE4/Assets/Exports/Texture/ULightMapVirtualTexture2D.cs
// A lightmap stored as a virtual texture; the VT payload is read by FTexturePlatformData.
#pragma once

#include "UTexture2D.h"

namespace CUE4Parse::UE4::Assets::Exports::Texture
{
    class ULightMapVirtualTexture2D : public UTexture2D
    {
    };
}
