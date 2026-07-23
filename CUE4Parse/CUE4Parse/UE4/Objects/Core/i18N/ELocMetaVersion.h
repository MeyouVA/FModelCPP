// Ported from CUE4Parse/UE4/Objects/Core/i18N/ELocMetaVersion.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Objects::Core::i18N
{
    enum class ELocMetaVersion : uint8_t
    {
        // Initial format.
        Initial               = 0,
        // Added complete list of cultures compiled for the localization target.
        AddedCompiledCultures,
        // Added bIsUGC flag
        AddedIsUGC,
        LatestPlusOne,
        Latest                = LatestPlusOne - 1,
    };
}
