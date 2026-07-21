// Ported from CUE4Parse/UE4/Assets/Exports/Engine/ECurveTableMode.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Assets::Exports::Engine
{
    enum class ECurveTableMode : uint8_t
    {
        Empty,
        SimpleCurves,
        RichCurves
    };
}
