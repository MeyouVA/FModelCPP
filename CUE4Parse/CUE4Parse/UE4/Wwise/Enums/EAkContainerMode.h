// Ported from CUE4Parse/UE4/Wwise/Enums/EAkContainerMode.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums
{
    enum class EAkContainerMode : uint8_t
    {
        Random,
        Sequence,
    };
}
