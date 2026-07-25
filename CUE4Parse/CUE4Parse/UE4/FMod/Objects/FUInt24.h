// Ported from CUE4Parse/UE4/FMod/Objects/FUInt24.cs
// A 24-bit unsigned value widened into a uint32, as read by FModReader::ReadSimpleArray24.
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::FMod::Objects
{
    struct FUInt24
    {
        uint32_t Value = 0;

        FUInt24() = default;
        explicit FUInt24(uint32_t v) : Value(v) {}
    };
}
