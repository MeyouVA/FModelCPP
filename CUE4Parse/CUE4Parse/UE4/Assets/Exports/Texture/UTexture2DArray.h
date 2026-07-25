// Ported from CUE4Parse/UE4/Assets/Exports/Texture/UTexture2DArray.cs
// An array of 2D textures. Unlike a cubemap or a volume texture it does NOT get its mips restacked -- the
// slice count stays in PackedData and the mips are read as-is.
#pragma once

#include <cstdint>

#include "TextureAddress.h"
#include "UTexture.h"
#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Exports::Texture
{
    class UTexture2DArray : public UTexture
    {
    public:
        TextureAddress AddressX{};
        TextureAddress AddressY{};
        TextureAddress AddressZ{};

        TextureAddress GetTextureAddressX() const override { return AddressX; }
        TextureAddress GetTextureAddressY() const override { return AddressY; }
        TextureAddress GetTextureAddressZ() const override { return AddressZ; }

        void Deserialize(Readers::FAssetArchive& Ar, int64_t validPos) override;
    };
}
