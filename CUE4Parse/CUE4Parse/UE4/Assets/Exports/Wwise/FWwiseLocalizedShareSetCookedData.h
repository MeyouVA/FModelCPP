// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/FWwiseLocalizedShareSetCookedData.cs
// The per-language variants of one share set: the same asset cooked once per localisation.
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
#include "FWwiseShareSetCookedData.h"
#include "FWwiseLocalizedCookedDataMap.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;
    using CUE4Parse::UE4::Objects::UObject::FName;

    struct FWwiseLocalizedShareSetCookedData
    {
        std::map<FWwiseLanguageCookedData, std::optional<FWwiseShareSetCookedData>> ShareSetLanguageMap;
        FName DebugName;
        int32_t ShareSetId = 0;

        FWwiseLocalizedShareSetCookedData() = default;

        explicit FWwiseLocalizedShareSetCookedData(const FStructFallback& fallback)
        {
            ShareSetLanguageMap = ReadLanguageMap<FWwiseShareSetCookedData>(fallback, "ShareSetLanguageMap");
            DebugName = PropertyUtil::GetOrDefault<FName>(fallback, "DebugName");
            ShareSetId = PropertyUtil::GetOrDefault<int32_t>(fallback, "ShareSetId");
        }

        void SerializeBulkData(FAssetArchive& Ar) const
        {
            for (const auto& lang : ShareSetLanguageMap)
            {
                if (lang.second.has_value()) lang.second->SerializeBulkData(Ar);
            }
        }
    };
}
