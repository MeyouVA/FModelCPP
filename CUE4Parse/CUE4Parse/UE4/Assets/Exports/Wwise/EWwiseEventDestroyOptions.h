// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/EWwiseEventDestroyOptions.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    enum class EWwiseEventDestroyOptions : uint8_t
    {
        StopEventOnDestroy            = 0,
        WaitForEventEnd               = 1,
        EWwiseEventDestroyOptions_MAX = 2,
    };
}
