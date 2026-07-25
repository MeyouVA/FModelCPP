// Ported from CUE4Parse/UE4/Assets/Exports/Texture/UTextureAllMipDataProviderFactory.cs
// The "I can supply every mip" flavour, which is the one UTexture.MipDataProvider looks for.
#pragma once

#include "UTextureMipDataProviderFactory.h"

namespace CUE4Parse::UE4::Assets::Exports::Texture
{
    class UTextureAllMipDataProviderFactory : public UTextureMipDataProviderFactory
    {
    };
}
