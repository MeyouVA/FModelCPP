// Ported from CUE4Parse/UE4/FMod/Enums/EEvaluatorType.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::FMod::Enums
{
    enum class EEvaluatorType : uint8_t
    {
        Basic0 = 0,
        Basic1 = 1,
        Basic2 = 2,
        Basic3 = 3,
        Type10 = 0x10,
        Type11 = 0x11,
        Type12 = 0x12,
        Type20 = 0x20,
        Type30 = 0x30,
    };
}
