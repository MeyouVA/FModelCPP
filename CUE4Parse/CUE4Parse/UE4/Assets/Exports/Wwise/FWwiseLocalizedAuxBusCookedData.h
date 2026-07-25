// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/FWwiseLocalizedAuxBusCookedData.cs
// The per-language variants of one auxiliary bus: the same asset cooked once per localisation.
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
#include "FWwiseAuxBusCookedData.h"
#include "FWwiseLocalizedCookedDataMap.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;
    using CUE4Parse::UE4::Objects::UObject::FName;

    struct FWwiseLocalizedAuxBusCookedData
    {
        std::map<FWwiseLanguageCookedData, std::optional<FWwiseAuxBusCookedData>> AuxBusLanguageMap;
        FName DebugName;
        int32_t AuxBusId = 0;

        FWwiseLocalizedAuxBusCookedData() = default;

        explicit FWwiseLocalizedAuxBusCookedData(const FStructFallback& fallback)
        {
            AuxBusLanguageMap = ReadLanguageMap<FWwiseAuxBusCookedData>(fallback, "AuxBusLanguageMap");
            DebugName = PropertyUtil::GetOrDefault<FName>(fallback, "DebugName");
            AuxBusId = PropertyUtil::GetOrDefault<int32_t>(fallback, "AuxBusId");
        }

        void SerializeBulkData(FAssetArchive& Ar) const
        {
            for (const auto& lang : AuxBusLanguageMap)
            {
                if (lang.second.has_value()) lang.second->SerializeBulkData(Ar);
            }
        }
    };
}
