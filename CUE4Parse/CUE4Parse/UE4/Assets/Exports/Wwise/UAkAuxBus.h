// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/UAkAuxBus.cs
// An auxiliary bus asset; the attenuation radius follows the cooked data on the wire.
#pragma once

#include <optional>

#include "../../Objects/FStructFallback.h"
#include "../../Readers/FAssetArchive.h"
#include "UAkAudioType.h"
#include "FWwiseLocalizedAuxBusCookedData.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;

    class UAkAuxBus : public UAkAudioType
    {
    public:
        std::optional<FWwiseLocalizedAuxBusCookedData> AuxBusCookedData;
        float MaxAttenuationRadius = 0.0f;

        void Deserialize(FAssetArchive& Ar, int64_t validPos) override
        {
            UAkAudioType::Deserialize(Ar, validPos);

            if (Ar.Position >= validPos) return;
            AuxBusCookedData.emplace(FStructFallback(Ar, std::string("WwiseLocalizedAuxBusCookedData")));
            AuxBusCookedData->SerializeBulkData(Ar);
            MaxAttenuationRadius = Ar.Read<float>();
        }
    };
}
