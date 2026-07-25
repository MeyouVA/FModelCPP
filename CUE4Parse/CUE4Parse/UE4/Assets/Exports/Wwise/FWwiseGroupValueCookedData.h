// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/FWwiseGroupValueCookedData.cs
// One switch/state value a cooked event depends on.
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../PropertyUtil.h"
#include "../../Objects/FStructFallback.h"
#include "../../Readers/FAssetArchive.h"
#include "EWwiseGroupType.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;
    using CUE4Parse::UE4::Objects::UObject::FName;

    struct FWwiseGroupValueCookedData
    {
        EWwiseGroupType Type = static_cast<EWwiseGroupType>(0);
        int32_t GroupId = 0;
        int32_t ID = 0;
        FName DebugName;

        FWwiseGroupValueCookedData() = default;

        explicit FWwiseGroupValueCookedData(const FStructFallback& fallback)
        {
            Type = PropertyUtil::GetOrDefault<EWwiseGroupType>(fallback, "Type");
            GroupId = PropertyUtil::GetOrDefault<int32_t>(fallback, "GroupId");
            ID = PropertyUtil::GetOrDefault<int32_t>(fallback, "ID");
            DebugName = PropertyUtil::GetOrDefault<FName>(fallback, "DebugName");
        }
    };
}
