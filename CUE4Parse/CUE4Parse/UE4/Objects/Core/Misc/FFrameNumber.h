// Ported from CUE4Parse/UE4/Objects/Core/Misc/FFrameNumber.cs
#pragma once

#include <cstdint>
#include <string>

#include "../../../IUStruct.h"

namespace CUE4Parse::UE4::Objects::Core::Misc
{
    struct FFrameNumber : public UE4::IUStruct
    {
        int32_t Value = 0;

        FFrameNumber() = default;
        FFrameNumber(int32_t value) : Value(value) {}
        // Mirrors the implicit conversion from float (truncates).
        FFrameNumber(float value) : Value(static_cast<int32_t>(value)) {}

        std::string ToString() const { return std::to_string(Value); }
    };
}
