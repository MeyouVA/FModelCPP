// Ported from CUE4Parse/UE4/Assets/Exports/Texture/UPaperSprite.cs
// A Paper2D sprite. Not a texture at all -- it derives straight from UObject and only *points* at one --
// but it lives in this folder in the C# and so it lives here too.
#pragma once

#include <cstdint>
#include <vector>

#include "../PropertyUtil.h"
#include "../UObject.h"
#include "../../Readers/FAssetArchive.h"
#include "../../../Objects/Core/Math/FVector2D.h"
#include "../../../Objects/Core/Math/FVector4.h"
#include "../../../Objects/UObject/ObjectResource.h"

namespace CUE4Parse::UE4::Assets::Exports::Texture
{
    using CUE4Parse::UE4::Objects::Core::Math::FVector2D;
    using CUE4Parse::UE4::Objects::Core::Math::FVector4;
    using CUE4Parse::UE4::Objects::UObject::FPackageIndex;

    class UPaperSprite : public UObject
    {
    public:
        FVector2D BakedSourceUV;
        FVector2D BakedSourceDimension;
        FPackageIndex BakedSourceTexture;
        FPackageIndex DefaultMaterial;
        float PixelsPerUnrealUnit = 0.0f;
        std::vector<FVector4> BakedRenderData;

        void Deserialize(Readers::FAssetArchive& Ar, int64_t validPos) override
        {
            UObject::Deserialize(Ar, validPos);

            BakedSourceUV = PropertyUtil::GetOrDefault<FVector2D>(*this, "BakedSourceUV", FVector2D::ZeroVector);
            BakedSourceDimension = PropertyUtil::GetOrDefault<FVector2D>(*this, "BakedSourceDimension", FVector2D::ZeroVector);
            BakedSourceTexture = PropertyUtil::GetOrDefault<FPackageIndex>(*this, "BakedSourceTexture");
            DefaultMaterial = PropertyUtil::GetOrDefault<FPackageIndex>(*this, "DefaultMaterial");
            PixelsPerUnrealUnit = PropertyUtil::GetOrDefault<float>(*this, "PixelsPerUnrealUnit", 1.0f);
            BakedRenderData = PropertyUtil::GetArray<FVector4>(*this, "BakedRenderData");
        }
    };
}
