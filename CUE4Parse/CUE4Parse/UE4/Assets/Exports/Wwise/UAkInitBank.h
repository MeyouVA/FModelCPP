// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/UAkInitBank.cs
// The Init bank asset.
#pragma once

#include <optional>

#include "../../Objects/FStructFallback.h"
#include "../../Readers/FAssetArchive.h"
#include "UAkAudioType.h"
#include "FWwiseInitBankCookedData.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;

    class UAkInitBank : public UAkAudioType
    {
    public:
        std::optional<FWwiseInitBankCookedData> InitBankCookedData;

        void Deserialize(FAssetArchive& Ar, int64_t validPos) override
        {
            UAkAudioType::Deserialize(Ar, validPos);

            if (Ar.Position >= validPos) return;
            InitBankCookedData.emplace(FStructFallback(Ar, std::string("WwiseInitBankCookedData")));
            InitBankCookedData->SerializeBulkData(Ar);
        }
    };
}
