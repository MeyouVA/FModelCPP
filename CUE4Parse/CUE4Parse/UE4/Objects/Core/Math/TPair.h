// Ported from CUE4Parse/UE4/Objects/Core/Math/TPair.cs — a generic value pair.
#pragma once

#include <string>

namespace CUE4Parse::UE4::Objects::Core::Math
{
    template <typename T>
    struct TPair
    {
        T X{};
        T Y{};

        TPair() = default;
        TPair(T x, T y) : X(x), Y(y) {}

        std::string ToString() const
        {
            return "X: " + std::to_string(X) + ", Y: " + std::to_string(Y);
        }
    };
}
