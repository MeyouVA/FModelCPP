// Ported from CUE4Parse/UE4/Assets/Exports/Texture/UVolumeTexture.cs
// A 3D texture. Like a cubemap it is cooked as one tall 2D image; FTexturePlatformData stacks the slices
// and, uniquely for this type, writes the slice count back into PackedData from mip 0's SizeZ.
#pragma once

#include <cstdint>

#include "TextureAddress.h"
#include "UTexture.h"
#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Exports::Texture
{
    class UVolumeTexture : public UTexture
    {
    public:
        TextureAddress AddressMode{};

        TextureAddress GetTextureAddressX() const override { return AddressMode; }
        TextureAddress GetTextureAddressY() const override { return AddressMode; }
        TextureAddress GetTextureAddressZ() const override { return AddressMode; }

        void Deserialize(Readers::FAssetArchive& Ar, int64_t validPos) override;
    };
}
