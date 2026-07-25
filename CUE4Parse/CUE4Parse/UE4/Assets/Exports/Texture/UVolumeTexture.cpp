#include "UVolumeTexture.h"

#include "../PropertyUtil.h"
#include "../../../Objects/Engine/FStripDataFlags.h"

namespace CUE4Parse::UE4::Assets::Exports::Texture
{
    using CUE4Parse::UE4::Objects::Engine::FStripDataFlags;

    void UVolumeTexture::Deserialize(Readers::FAssetArchive& Ar, int64_t validPos)
    {
        UTexture::Deserialize(Ar, validPos);

        AddressMode = PropertyUtil::GetOrDefault<TextureAddress>(*this, "AddressMode");

        const FStripDataFlags stripFlags(Ar);
        const bool bCooked = Ar.ReadBoolean();

        if (bCooked)
        {
            DeserializeCookedPlatformData(Ar);
        }
    }
}
