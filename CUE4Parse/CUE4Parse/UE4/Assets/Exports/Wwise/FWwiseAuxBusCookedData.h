// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/FWwiseAuxBusCookedData.cs
// The banks and media one auxiliary bus needs loaded, for a single language.
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../PropertyUtil.h"
#include "../../Objects/FStructFallback.h"
#include "../../Readers/FAssetArchive.h"
#include "FWwiseMediaCookedData.h"
#include "FWwiseSoundBankCookedData.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;
    using CUE4Parse::UE4::Objects::UObject::FName;

    struct FWwiseAuxBusCookedData
    {
        int32_t AuxBusId = 0;
        std::vector<FWwiseSoundBankCookedData> SoundBanks;
        std::vector<FWwiseMediaCookedData> Media;
        FName DebugName;

        FWwiseAuxBusCookedData() = default;

        explicit FWwiseAuxBusCookedData(const FStructFallback& fallback)
        {
            AuxBusId = PropertyUtil::GetOrDefault<int32_t>(fallback, "AuxBusId");
            SoundBanks = PropertyUtil::GetStructArray<FWwiseSoundBankCookedData>(fallback, "SoundBanks");
            Media = PropertyUtil::GetStructArray<FWwiseMediaCookedData>(fallback, "Media");
            DebugName = PropertyUtil::GetOrDefault<FName>(fallback, "DebugName");
        }

        void SerializeBulkData(FAssetArchive& Ar) const
        {
            for (const auto& sb : SoundBanks) sb.SerializeBulkData(Ar);
            for (const auto& media : Media) media.SerializeBulkData(Ar);
        }
    };
}
