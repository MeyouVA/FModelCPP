#include "UTexture2D.h"

#include "FTexture2DMipMap.h"
#include "../PropertyUtil.h"
#include "../../../Objects/Core/Misc/FGuid.h"
#include "../../../Objects/Engine/FStripDataFlags.h"
#include "../../../Versions/EGame.h"
#include "../../../Versions/ObjectVersion.h"

namespace CUE4Parse::UE4::Assets::Exports::Texture
{
    using CUE4Parse::UE4::Objects::Core::Misc::FGuid;
    using CUE4Parse::UE4::Objects::Engine::FStripDataFlags;
    using CUE4Parse::UE4::Versions::EUnrealEngineObjectUE4Version;
    using namespace CUE4Parse::UE4::Versions;

    void UTexture2D::Deserialize(Readers::FAssetArchive& Ar, int64_t validPos)
    {
        if (Ar.Game() == GAME_WorldofJadeDynasty) Ar.Position += 12;
        UTexture::Deserialize(Ar, validPos);
        ImportedSize = PropertyUtil::GetOrDefault<FIntPoint>(*this, "ImportedSize");
        AddressX = PropertyUtil::GetOrDefault<TextureAddress>(*this, "AddressX");
        AddressY = PropertyUtil::GetOrDefault<TextureAddress>(*this, "AddressY");

        const FStripDataFlags stripDataFlags(Ar);
        const bool bCooked = Ar.Ver() >= EUnrealEngineObjectUE4Version::ADD_COOKED_TO_TEXTURE2D && Ar.ReadBoolean();
        if (Ar.Ver() < EUnrealEngineObjectUE4Version::TEXTURE_SOURCE_ART_REFACTOR)
        {
            // C#: Log.Warning("Untested code: UTexture2D::LegacySerialize"). The port has no logging layer.
            // https://github.com/EpicGames/UnrealEngine/blob/2092a941a52c55750072f24cd4757176dfaa8326/Engine/Source/Runtime/Engine/Private/Texture2D.cpp

            std::vector<FTexture2DMipMap> legacyMips;

            const bool bHasLegacyMips = PropertyUtil::GetOrDefault<bool>(*this, "bDisableDerivedDataCache_DEPRECATED", false);
            if (bHasLegacyMips)
            {
                legacyMips = Ar.ReadArrayWith([&Ar] { return FTexture2DMipMap(Ar); });
            }

            Ar.Read<FGuid>(); // textureFileCacheGuidDeprecated

            Format = PropertyUtil::GetOrDefault<EPixelFormat>(*this, "Format", EPixelFormat::PF_Unknown);

            if (bHasLegacyMips && !legacyMips.empty())
            {
                // TODO: Populate PlatformData.Mips[] with LegacyMips data.
            }
        }

        if (bCooked)
        {
            bool bSerializeMipData = true;
            if (Ar.Game() >= GAME_UE5_3 || Ar.Game() == GAME_TheFirstDescendant)
            {
                // Controls whether FByteBulkData is serialized??
                bSerializeMipData = Ar.ReadBoolean();
            }

            if (Ar.Position >= validPos) return;

            DeserializeCookedPlatformData(Ar, bSerializeMipData);
        }
    }
}
