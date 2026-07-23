// Ported from CUE4Parse/UE4/Assets/Exports/Animation/EAnimInterpolationType.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Assets::Exports::Animation
{
    enum class EAnimInterpolationType : uint8_t
    {
        Linear,
        Step,
    };
}
