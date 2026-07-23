// Ported from CUE4Parse/UE4/Objects/Core/Math/FIntVector2.cs.
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Objects::Core::Math
{
    struct FIntVector2
    {
        int32_t X = 0;
        int32_t Y = 0;

        FIntVector2() = default;
        FIntVector2(int32_t x, int32_t y) : X(x), Y(y) {}
    };
}
