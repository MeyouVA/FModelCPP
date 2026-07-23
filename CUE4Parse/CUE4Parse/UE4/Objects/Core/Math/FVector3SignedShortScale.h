// Ported from CUE4Parse/UE4/Objects/Core/Math/FVector3SignedShortScale.cs.
// Packed vertex-normal / position formats that decode to an FVector.
#pragma once

#include <cstdint>

#include "FVector.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    struct FVector3SignedShortScale
    {
        int16_t X = 0;
        int16_t Y = 0;
        int16_t Z = 0;
        int16_t W = 0;

        FVector3SignedShortScale() = default;
        FVector3SignedShortScale(int16_t x, int16_t y, int16_t z, int16_t w) : X(x), Y(y), Z(z), W(w) {}

        operator FVector() const
        {
            // W == short.MaxValue suggests it should be used as the scale; fall back to 1 when zero.
            const float wf = W == 0 ? 1.0f : static_cast<float>(W);
            return FVector(X / wf, Y / wf, Z / wf);
        }
    };

    struct FVector3UnsignedShortScale
    {
        uint16_t X = 0;
        uint16_t Y = 0;
        uint16_t Z = 0;
        uint16_t W = 0;

        FVector3UnsignedShortScale() = default;
        FVector3UnsignedShortScale(uint16_t x, uint16_t y, uint16_t z, uint16_t w) : X(x), Y(y), Z(z), W(w) {}

        operator FVector() const { return FVector(static_cast<float>(X), static_cast<float>(Y), static_cast<float>(Z)); }
    };

    struct FVector3Packed32
    {
        uint32_t Data = 0;

        float GetX() const { return static_cast<float>(Data & 0x3ff); }
        float GetY() const { return static_cast<float>((Data >> 10) & 0x3ff); }
        float GetZ() const { return static_cast<float>((Data >> 20) & 0x3ff); }

        operator FVector() const { return FVector(GetX(), GetY(), GetZ()); }
    };
}
