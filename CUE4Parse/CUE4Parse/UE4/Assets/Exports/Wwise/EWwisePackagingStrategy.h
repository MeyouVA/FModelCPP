// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/EWwisePackagingStrategy.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    enum class EWwisePackagingStrategy
    {
        Source               = 0,
        AdditionalFile       = 1,
        HybridAdditionalFile = 2,
        BulkData             = 3,
        External             = 4,
    };
}
