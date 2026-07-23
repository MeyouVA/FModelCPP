// Ported from CUE4Parse/UE4/Objects/Core/Math/FIntPoint.cs.
#pragma once

#include <cstdint>
#include <string>

namespace CUE4Parse::UE4::Objects::Core::Math
{
    struct FIntPoint
    {
        int32_t X = 0;
        int32_t Y = 0;

        FIntPoint() = default;
        FIntPoint(int32_t x, int32_t y) : X(x), Y(y) {}

        std::string ToString() const { return "X: " + std::to_string(X) + ", Y: " + std::to_string(Y); }
    };

    struct FIntRect
    {
        FIntPoint Min;
        FIntPoint Max;

        FIntRect() = default;
        FIntRect(FIntPoint min, FIntPoint max) : Min(min), Max(max) {}
    };
}
