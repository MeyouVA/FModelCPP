// Ported from CUE4Parse/UE4/Assets/Exports/Texture/UTextureProFX.cs (both classes it declares).
// A per-game texture type whose payload is a ".map" blob rather than a mip chain, so it bypasses
// FTexturePlatformData entirely: dimensions and format come off the wire and the bytes stay in RawData.
#pragma once

#include <cstdint>
#include <optional>

#include "PixelFormat.h"
#include "UTexture.h"
#include "../../Objects/FByteBulkData.h"
#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Exports::Texture
{
    using CUE4Parse::UE4::Assets::Objects::FByteBulkData;

    // RawData is the data of a ".map" format
    class UTextureProFXParent : public UTexture
    {
    public:
        int32_t SizeX = 0;
        int32_t SizeY = 0;
        std::optional<FByteBulkData> RawData;

        void Deserialize(Readers::FAssetArchive& Ar, int64_t validPos) override
        {
            UTexture::Deserialize(Ar, validPos);
            SizeX = Ar.Read<int32_t>();
            SizeY = Ar.Read<int32_t>();
            Format = static_cast<EPixelFormat>(Ar.Read<int32_t>());
            RawData.emplace(Ar);
        }
    };

    class UTextureProFXChild : public UTexture
    {
    public:
        int32_t SizeX = 0;
        int32_t SizeY = 0;

        void Deserialize(Readers::FAssetArchive& Ar, int64_t validPos) override
        {
            UTexture::Deserialize(Ar, validPos);
            SizeX = Ar.Read<int32_t>();
            SizeY = Ar.Read<int32_t>();
            Format = static_cast<EPixelFormat>(Ar.Read<int32_t>());
        }
    };
}
