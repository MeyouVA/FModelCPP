#include "UTexture2DArray.h"

#include "../PropertyUtil.h"
#include "../../../Objects/Engine/FStripDataFlags.h"
#include "../../../Versions/EGame.h"

namespace CUE4Parse::UE4::Assets::Exports::Texture
{
    using CUE4Parse::UE4::Objects::Engine::FStripDataFlags;
    using namespace CUE4Parse::UE4::Versions;

    void UTexture2DArray::Deserialize(Readers::FAssetArchive& Ar, int64_t validPos)
    {
        UTexture::Deserialize(Ar, validPos);
        if (Ar.Game() == GAME_WorldofJadeDynasty) Ar.Position += 24;
        AddressX = PropertyUtil::GetOrDefault<TextureAddress>(*this, "AddressX");
        AddressY = PropertyUtil::GetOrDefault<TextureAddress>(*this, "AddressY");
        AddressZ = PropertyUtil::GetOrDefault<TextureAddress>(*this, "AddressZ");

        const FStripDataFlags stripFlags(Ar);
        const bool bCooked = Ar.ReadBoolean();

        if (bCooked)
        {
            DeserializeCookedPlatformData(Ar);
        }
    }
}
