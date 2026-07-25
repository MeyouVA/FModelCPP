// Ported from CUE4Parse/UE4/Assets/Exports/Texture/UTextureLightProfile.cs
// An IES light profile stored as a texture. Adds only two scalars, both read from tagged properties.
#pragma once

#include <cstdint>

#include "UTexture2D.h"
#include "../PropertyUtil.h"
#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Exports::Texture
{
    class UTextureLightProfile : public UTexture2D
    {
    public:
        float Brightness = 0.0f;
        float TextureMultiplier = 0.0f;

        void Deserialize(Readers::FAssetArchive& Ar, int64_t validPos) override
        {
            UTexture2D::Deserialize(Ar, validPos);

            Brightness = PropertyUtil::GetOrDefault<float>(*this, "Brightness", -1.0f);
            TextureMultiplier = PropertyUtil::GetOrDefault<float>(*this, "TextureMultiplier", 1.0f);
        }
    };
}
