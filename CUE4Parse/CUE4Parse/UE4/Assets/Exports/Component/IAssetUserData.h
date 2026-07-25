// Ported from CUE4Parse/UE4/Assets/Exports/Component/IAssetUserData.cs
// "This export carries an AssetUserData array." A marker interface in C#, so a pure-virtual accessor here.
// UTexture implements it; the mip-data-provider lookup is the one consumer in this part of the tree.
#pragma once

#include <vector>

#include "../../../Objects/UObject/ObjectResource.h"

namespace CUE4Parse::UE4::Assets::Exports::Component
{
    using CUE4Parse::UE4::Objects::UObject::FPackageIndex;

    class IAssetUserData
    {
    public:
        virtual ~IAssetUserData() = default;
        virtual const std::vector<FPackageIndex>& GetAssetUserData() const = 0;
    };
}
