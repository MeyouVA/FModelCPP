// Ported from CUE4Parse/UE4/Objects/Core/Math/FIntVector.cs.
#pragma once

#include <cstdint>
#include <string>

namespace CUE4Parse::UE4::Objects::Core::Math
{
    struct FIntVector
    {
        int32_t X = 0;
        int32_t Y = 0;
        int32_t Z = 0;

        static FIntVector Zero() { return FIntVector(0, 0, 0); }

        FIntVector() = default;
        FIntVector(int32_t x, int32_t y, int32_t z) : X(x), Y(y), Z(z) {}

        std::string ToString() const
        {
            return "X: " + std::to_string(X) + ", Y: " + std::to_string(Y) + ", Z: " + std::to_string(Z);
        }

        // Same-type / int-bias arithmetic. The mixed FVector-returning overloads from the C# source are
        // deferred (they are not used on the parse path and would couple this leaf type to FVector).
        friend FIntVector operator+(FIntVector a, FIntVector b) { return {a.X + b.X, a.Y + b.Y, a.Z + b.Z}; }
        friend FIntVector operator+(FIntVector a, int32_t bias) { return {a.X + bias, a.Y + bias, a.Z + bias}; }
        friend FIntVector operator-(FIntVector a, FIntVector b) { return {a.X - b.X, a.Y - b.Y, a.Z - b.Z}; }
        friend FIntVector operator-(FIntVector a, int32_t bias) { return {a.X - bias, a.Y - bias, a.Z - bias}; }
        friend FIntVector operator*(FIntVector a, FIntVector b) { return {a.X * b.X, a.Y * b.Y, a.Z * b.Z}; }
        friend FIntVector operator*(FIntVector a, int32_t bias) { return {a.X * bias, a.Y * bias, a.Z * bias}; }
        friend FIntVector operator/(FIntVector a, FIntVector b) { return {a.X / b.X, a.Y / b.Y, a.Z / b.Z}; }
        friend FIntVector operator/(FIntVector a, int32_t bias) { return {a.X / bias, a.Y / bias, a.Z / bias}; }
    };
}
