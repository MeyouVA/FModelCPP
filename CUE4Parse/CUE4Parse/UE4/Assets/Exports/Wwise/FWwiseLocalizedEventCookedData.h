// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/FWwiseLocalizedEventCookedData.cs
// The per-language variants of one event: the same asset cooked once per localisation.
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
#include "FWwiseEventCookedData.h"
#include "FWwiseLocalizedCookedDataMap.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;
    using CUE4Parse::UE4::Objects::UObject::FName;

    struct FWwiseLocalizedEventCookedData
    {
        std::map<FWwiseLanguageCookedData, std::optional<FWwiseEventCookedData>> EventLanguageMap;
        FName DebugName;
        uint32_t EventId = 0;

        FWwiseLocalizedEventCookedData() = default;

        explicit FWwiseLocalizedEventCookedData(const FStructFallback& fallback)
        {
            EventLanguageMap = ReadLanguageMap<FWwiseEventCookedData>(fallback, "EventLanguageMap");
            DebugName = PropertyUtil::GetOrDefault<FName>(fallback, "DebugName");
            EventId = static_cast<uint32_t>(PropertyUtil::GetOrDefault<int32_t>(fallback, "EventId"));
        }

        void SerializeBulkData(FAssetArchive& Ar) const
        {
            for (const auto& lang : EventLanguageMap)
            {
                if (lang.second.has_value()) lang.second->SerializeBulkData(Ar);
            }
        }
    };
}
