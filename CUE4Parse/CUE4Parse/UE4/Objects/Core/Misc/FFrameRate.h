// Ported from CUE4Parse/UE4/Objects/Core/Misc/FFrameRate.cs
#pragma once

#include <cstdint>
#include <string>

#include "../../../IUStruct.h"

namespace CUE4Parse::UE4::Objects::Core::Misc
{
    struct FFrameRate : public UE4::IUStruct
    {
        int32_t Numerator = 0;
        int32_t Denominator = 0;

        FFrameRate() = default;
        FFrameRate(int32_t numerator, int32_t denominator) : Numerator(numerator), Denominator(denominator) {}

        std::string ToString() const
        {
            return "Numerator: " + std::to_string(Numerator) + ", Denominator: " + std::to_string(Denominator);
        }
    };
}
