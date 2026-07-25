// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/UAkEffectShareSet.cs
// An effect share set asset.
#pragma once

#include <optional>

#include "../../Objects/FStructFallback.h"
#include "../../Readers/FAssetArchive.h"
#include "UAkAudioType.h"
#include "FWwiseLocalizedShareSetCookedData.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;

    class UAkEffectShareSet : public UAkAudioType
    {
    public:
        std::optional<FWwiseLocalizedShareSetCookedData> ShareSetCookedData;

        void Deserialize(FAssetArchive& Ar, int64_t validPos) override
        {
            UAkAudioType::Deserialize(Ar, validPos);

            if (Ar.Position >= validPos) return;
            ShareSetCookedData.emplace(FStructFallback(Ar, std::string("WwiseLocalizedShareSetCookedData")));
            ShareSetCookedData->SerializeBulkData(Ar);
        }
    };
}
