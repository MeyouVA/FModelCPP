// Ported from CUE4Parse/UE4/Assets/Exports/Texture/UTexture2D.cs
// The common case: a plain 2D texture. Everything interesting is inherited; this class adds the two address
// modes, the imported size, and the decision of whether there is cooked platform data to read at all.
#pragma once

#include <cstdint>

#include "TextureAddress.h"
#include "UTexture.h"
#include "../../Readers/FAssetArchive.h"
#include "../../../Objects/Core/Math/FIntPoint.h"

namespace CUE4Parse::UE4::Assets::Exports::Texture
{
    using CUE4Parse::UE4::Objects::Core::Math::FIntPoint;

    class UTexture2D : public UTexture
    {
    public:
        FIntPoint ImportedSize;
        TextureAddress AddressX{};
        TextureAddress AddressY{};

        TextureAddress GetTextureAddressX() const override { return AddressX; }
        TextureAddress GetTextureAddressY() const override { return AddressY; }

        void Deserialize(Readers::FAssetArchive& Ar, int64_t validPos) override;
    };
}
