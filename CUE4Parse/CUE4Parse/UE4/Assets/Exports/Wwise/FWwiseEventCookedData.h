// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/FWwiseEventCookedData.cs
// Everything one Wwise event needs at runtime, for a single language.
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../PropertyUtil.h"
#include "../../Objects/FStructFallback.h"
#include "../../Objects/UScriptMap.h"
#include "../../Objects/Properties/MapProperty.h"
#include "../../Objects/UScriptSet.h"
#include "../../Readers/FAssetArchive.h"
#include "EWwiseEventDestroyOptions.h"
#include "FWwiseExternalSourceCookedData.h"
#include "FWwiseMediaCookedData.h"
#include "FWwiseSoundBankCookedData.h"
#include "FWwiseSwitchContainerLeafCookedData.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Assets::Objects::UScriptSet;
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;
    using CUE4Parse::UE4::Objects::UObject::FName;

    struct FWwiseEventCookedData
    {
        uint32_t EventId = 0;
        std::vector<FWwiseSoundBankCookedData> SoundBanks;
        std::vector<FWwiseMediaCookedData> Media;
        std::vector<FWwiseExternalSourceCookedData> ExternalSources;
        std::vector<FWwiseSwitchContainerLeafCookedData> SwitchContainerLeaves;
        const UScriptSet* RequiredGroupValueSet = nullptr; // FWwiseGroupValueCookedData[]
        EWwiseEventDestroyOptions DestroyOptions = static_cast<EWwiseEventDestroyOptions>(0);
        FName DebugName;

        FWwiseEventCookedData() = default;

        explicit FWwiseEventCookedData(const FStructFallback& fallback)
        {
            EventId = static_cast<uint32_t>(PropertyUtil::GetOrDefault<int32_t>(fallback, "EventId"));
            SoundBanks = PropertyUtil::GetStructArray<FWwiseSoundBankCookedData>(fallback, "SoundBanks");
            Media = PropertyUtil::GetStructArray<FWwiseMediaCookedData>(fallback, "Media");
            ExternalSources = PropertyUtil::GetStructArray<FWwiseExternalSourceCookedData>(fallback, "ExternalSources");
            SwitchContainerLeaves = PropertyUtil::GetStructArray<FWwiseSwitchContainerLeafCookedData>(
                fallback, "SwitchContainerLeaves");
            PropertyUtil::TryGet<const UScriptSet*>(fallback, "RequiredGroupValueSet", RequiredGroupValueSet);
            DestroyOptions = PropertyUtil::GetOrDefault<EWwiseEventDestroyOptions>(fallback, "DestroyOptions");
            DebugName = PropertyUtil::GetOrDefault<FName>(fallback, "DebugName");
        }

        void SerializeBulkData(FAssetArchive& Ar) const
        {
            for (const auto& sb : SoundBanks) sb.SerializeBulkData(Ar);
            for (const auto& media : Media) media.SerializeBulkData(Ar);
            for (const auto& leaf : SwitchContainerLeaves) leaf.SerializeBulkData(Ar);
        }
    };
}
