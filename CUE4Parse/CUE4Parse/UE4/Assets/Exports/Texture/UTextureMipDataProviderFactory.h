// Ported from CUE4Parse/UE4/Assets/Exports/Texture/UTextureMipDataProviderFactory.cs
// A texture may hand its mip bytes off to one of these instead of storing them inline; UTexture finds it by
// walking AssetUserData. The base carries no payload of its own -- the concrete subclass (e.g. the landscape
// one, not yet ported) is what actually produces bytes.
#pragma once

#include "../../../Objects/Engine/UAssetUserData.h"

namespace CUE4Parse::UE4::Assets::Exports::Texture
{
    class UTextureMipDataProviderFactory : public CUE4Parse::UE4::Objects::Engine::UAssetUserData
    {
    };
}
