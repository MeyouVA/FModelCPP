// Ported from CUE4Parse/UE4/Objects/Core/Math/TRange.cs — a lower/upper bound pair.
#pragma once

#include <string>

#include "TRangeBound.h"
#include "../../../IUStruct.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    template <typename T>
    struct TRange : public UE4::IUStruct
    {
        /** Holds the range's lower bound. */
        TRangeBound<T> LowerBound;
        /** Holds the range's upper bound. */
        TRangeBound<T> UpperBound;

        TRange() = default;
        TRange(TRangeBound<T> lowerBound, TRangeBound<T> upperBound)
            : LowerBound(lowerBound), UpperBound(upperBound) {}

        std::string ToString() const
        {
            return "LowerBound: " + LowerBound.ToString() + ", UpperBound: " + UpperBound.ToString();
        }
    };
}
