// Ported from CUE4Parse/UE4/Assets/Exports/Texture/UTextureCube.cs (both classes it declares).
// A cubemap's six faces are cooked into one image, so the mip chain is read exactly as for a 2D texture and
// FTexturePlatformData multiplies SizeY by the slice count instead. The six FacePosX/... indices are the
// uncooked editor references and are normally null in a shipped package.
#pragma once

#include <cstdint>

#include "PixelFormat.h"
#include "UTexture.h"
#include "../../Readers/FAssetArchive.h"
#include "../../../Objects/UObject/ObjectResource.h"

namespace CUE4Parse::UE4::Assets::Exports::Texture
{
    using CUE4Parse::UE4::Objects::UObject::FPackageIndex;

    class UTextureCube : public UTexture
    {
    public:
        FPackageIndex FacePosX;
        FPackageIndex FaceNegX;
        FPackageIndex FacePosY;
        FPackageIndex FaceNegY;
        FPackageIndex FacePosZ;
        FPackageIndex FaceNegZ;

        void Deserialize(Readers::FAssetArchive& Ar, int64_t validPos) override;
    };

    class UTextureCubeArray : public UTexture
    {
    public:
        void Deserialize(Readers::FAssetArchive& Ar, int64_t validPos) override;
    };
}
