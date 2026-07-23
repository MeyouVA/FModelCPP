// Ported from CUE4Parse/UE4/FMod/Enums/EQuantizationUnit.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::FMod::Enums
{
    enum class EQuantizationUnit : uint32_t
    {
        None       = 0x0,
        Bar        = 0x1,
        EighthNote = 0x2,
        Max        = 0x3,
    };
}
