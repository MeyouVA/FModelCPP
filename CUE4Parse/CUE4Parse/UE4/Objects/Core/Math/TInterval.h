// Ported from CUE4Parse/UE4/Objects/Core/Math/TInterval.cs — a generic [Min, Max] pair.
#pragma once

#include <string>

#include "../../../IUStruct.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    template <typename T>
    struct TInterval : public UE4::IUStruct
    {
        T Min{};
        T Max{};

        TInterval() = default;
        TInterval(T min, T max) : Min(min), Max(max) {}

        std::string ToString() const
        {
            return "Min: " + std::to_string(Min) + ", Max: " + std::to_string(Max);
        }
    };
}
