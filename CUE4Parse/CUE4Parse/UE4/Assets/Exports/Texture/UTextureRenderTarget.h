// Ported from CUE4Parse/UE4/Assets/Exports/Texture/UTextureRenderTarget.cs
// A render target has no cooked payload at all -- only the pre-UE3-refactor header, and nothing after it.
#pragma once

#include <cstdint>

#include "PixelFormat.h"
#include "UTexture.h"
#include "../../Readers/FAssetArchive.h"
#include "../../../Versions/ObjectVersion.h"

namespace CUE4Parse::UE4::Assets::Exports::Texture
{
    class UTextureRenderTarget : public UTexture
    {
    public:
        void Deserialize(Readers::FAssetArchive& Ar, int64_t validPos) override
        {
            UTexture::Deserialize(Ar, validPos);

            if (Ar.Ver() < CUE4Parse::UE4::Versions::EUnrealEngineObjectUE3Version::RENDERING_REFACTOR)
            {
                Ar.Read<int32_t>(); // SizeX
                Ar.Read<int32_t>(); // SizeY
                Format = static_cast<EPixelFormat>(Ar.Read<int32_t>());
                Ar.Read<int32_t>(); // numMips
            }
        }
    };
}
