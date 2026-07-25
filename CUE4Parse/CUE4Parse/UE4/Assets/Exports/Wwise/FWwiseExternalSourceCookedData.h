// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/FWwiseExternalSourceCookedData.cs
// A source the bank references but does not contain; the cookie is how the game resolves it at runtime.
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../PropertyUtil.h"
#include "../../Objects/FStructFallback.h"
#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;
    using CUE4Parse::UE4::Objects::UObject::FName;
    struct FWwiseExternalSourceCookedData
    {
        int32_t Cookie = 0;
        FName DebugName;

        FWwiseExternalSourceCookedData() = default;

        explicit FWwiseExternalSourceCookedData(const FStructFallback& fallback)
        {
            Cookie = PropertyUtil::GetOrDefault<int32_t>(fallback, "Cookie");
            DebugName = PropertyUtil::GetOrDefault<FName>(fallback, "DebugName");
        }
    };
}
