// Ported from CUE4Parse/UE4/Objects/Core/Math/TIntPoint.cs — a generic two-component point.
#pragma once

#include <string>

namespace CUE4Parse::UE4::Objects::Core::Math
{
    template <typename T>
    struct TIntPoint
    {
        T X{};
        T Y{};

        std::string ToString() const
        {
            return "X: " + std::to_string(X) + ", Y: " + std::to_string(Y);
        }
    };
}
