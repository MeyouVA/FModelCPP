// Ported from CUE4Parse/UE4/Wwise/Enums/EPositioningType.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums
{
    enum class EPositioningType : uint32_t
    {
        Undefined     = 0x0,
        Positioning2D = 0x1,
        UserDefined3D = 0x2,
        GameDefined3D = 0x3,
    };
}
