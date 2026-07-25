#include "UTextureCube.h"

#include "../PropertyUtil.h"
#include "../../../Objects/Engine/FStripDataFlags.h"
#include "../../../Versions/EGame.h"
#include "../../../Versions/ObjectVersion.h"

namespace CUE4Parse::UE4::Assets::Exports::Texture
{
    using CUE4Parse::UE4::Objects::Engine::FStripDataFlags;
    using CUE4Parse::UE4::Versions::EUnrealEngineObjectUE3Version;
    using namespace CUE4Parse::UE4::Versions;

    void UTextureCube::Deserialize(Readers::FAssetArchive& Ar, int64_t validPos)
    {
        UTexture::Deserialize(Ar, validPos);
        FacePosX = PropertyUtil::GetOrDefault<FPackageIndex>(*this, "FacePosX");
        FaceNegX = PropertyUtil::GetOrDefault<FPackageIndex>(*this, "FaceNegX");
        FacePosY = PropertyUtil::GetOrDefault<FPackageIndex>(*this, "FacePosY");
        FaceNegY = PropertyUtil::GetOrDefault<FPackageIndex>(*this, "FaceNegY");
        FacePosZ = PropertyUtil::GetOrDefault<FPackageIndex>(*this, "FacePosZ");
        FaceNegZ = PropertyUtil::GetOrDefault<FPackageIndex>(*this, "FaceNegZ");

        if (Ar.Ver() < EUnrealEngineObjectUE3Version::RENDERING_REFACTOR)
        {
            Ar.Read<int32_t>(); // SizeX
            Ar.Read<int32_t>(); // SizeY
            Format = static_cast<EPixelFormat>(Ar.Read<int32_t>());
            Ar.Read<int32_t>(); // numMips
        }

        if (Ar.Game() < GAME_UE4_0) return; // Nothing left
        const FStripDataFlags stripFlags(Ar);
        const bool bCooked = Ar.ReadBoolean();

        if (bCooked)
        {
            DeserializeCookedPlatformData(Ar);
        }
    }

    void UTextureCubeArray::Deserialize(Readers::FAssetArchive& Ar, int64_t validPos)
    {
        UTexture::Deserialize(Ar, validPos);

        const FStripDataFlags stripFlags(Ar);
        const bool bCooked = Ar.ReadBoolean();

        if (bCooked)
        {
            DeserializeCookedPlatformData(Ar);
        }
    }
}
