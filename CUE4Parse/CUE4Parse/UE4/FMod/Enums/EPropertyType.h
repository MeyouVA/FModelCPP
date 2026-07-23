// Ported from CUE4Parse/UE4/FMod/Enums/EPropertyType.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::FMod::Enums
{
    enum class EPropertyType : uint32_t
    {
        PropertyType_Normal    = 0x0,
        PropertyType_Volume    = 0x1,
        PropertyType_Undefined = 0x2,
        PropertyType_Max       = 0x3,
    };
}
