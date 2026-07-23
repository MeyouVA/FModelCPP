// Ported from CUE4Parse/UE4/Objects/Core/Math/FUIntVector.cs.
#pragma once

#include <cstdint>
#include <string>

namespace CUE4Parse::UE4::Objects::Core::Math
{
    struct FUIntVector
    {
        uint32_t X = 0;
        uint32_t Y = 0;
        uint32_t Z = 0;

        static FUIntVector Zero() { return FUIntVector(0u, 0u, 0u); }

        FUIntVector() = default;
        FUIntVector(uint32_t x, uint32_t y, uint32_t z) : X(x), Y(y), Z(z) {}
        FUIntVector(int32_t x, int32_t y, int32_t z)
            : X(static_cast<uint32_t>(x)), Y(static_cast<uint32_t>(y)), Z(static_cast<uint32_t>(z)) {}

        std::string ToString() const
        {
            return "X: " + std::to_string(X) + ", Y: " + std::to_string(Y) + ", Z: " + std::to_string(Z);
        }

        // Same-type / uint-bias arithmetic; the mixed FVector-returning overloads are deferred (see FIntVector).
        friend FUIntVector operator+(FUIntVector a, FUIntVector b) { return {a.X + b.X, a.Y + b.Y, a.Z + b.Z}; }
        friend FUIntVector operator+(FUIntVector a, uint32_t bias) { return {a.X + bias, a.Y + bias, a.Z + bias}; }
        friend FUIntVector operator*(FUIntVector a, FUIntVector b) { return {a.X * b.X, a.Y * b.Y, a.Z * b.Z}; }
        friend FUIntVector operator*(FUIntVector a, uint32_t bias) { return {a.X * bias, a.Y * bias, a.Z * bias}; }
        friend FUIntVector operator/(FUIntVector a, FUIntVector b) { return {a.X / b.X, a.Y / b.Y, a.Z / b.Z}; }
        friend FUIntVector operator/(FUIntVector a, uint32_t bias) { return {a.X / bias, a.Y / bias, a.Z / bias}; }
    };
}
