// Ported from CUE4Parse/Utils/MathUtils.cs
//
// Only the scalar helpers are ported here. The FVector/FVector2D/FVector4/FQuat <-> System.Numerics
// conversions and CubicCurve2D (which needs UnrealMath.IsNearlyZero) arrive with the Core/Math layer.
// C# extension methods become free functions in this namespace.
#pragma once

#include <cstdint>
#include <cstring>

namespace CUE4Parse::Utils
{
    namespace MathConstants
    {
        constexpr float PI_F = 3.14159265358979323846f;
    }

    // Fast inverse square root (Quake III), bit-for-bit as in C#.
    inline float InvSqrt(float x)
    {
        float xhalf = 0.5f * x;
        int32_t i;
        std::memcpy(&i, &x, sizeof(i));      // BitConverter.SingleToInt32Bits
        i = 0x5f3759df - (i >> 1);
        std::memcpy(&x, &i, sizeof(x));      // BitConverter.Int32BitsToSingle
        x = x * (1.5f - xhalf * x * x);
        return x;
    }

    inline int32_t DivideAndRoundUp(int32_t dividend, int32_t divisor) { return (dividend + divisor - 1) / divisor; }
    inline uint32_t DivideAndRoundUp(uint32_t dividend, uint32_t divisor) { return (dividend + divisor - 1u) / divisor; }

    inline float ToDegrees(float radVal) { return radVal * (180.0f / MathConstants::PI_F); }
    inline float ToRadians(float degVal) { return degVal * (MathConstants::PI_F / 180.0f); }

    inline float Square(float val) { return val * val; }

    inline int32_t TruncToInt(float f) { return static_cast<int32_t>(f); }
    inline int32_t TruncToInt(double f) { return static_cast<int32_t>(f); }
    inline int32_t FloorToInt(float f) { int32_t i = static_cast<int32_t>(f); return f < i ? i - 1 : i; } // Math.Floor then trunc
    inline int32_t RoundToInt(float f) { return FloorToInt(f + 0.5f); }

    inline int32_t Clamp(int32_t i, int32_t min, int32_t max) { return i < min ? min : i < max ? i : max; }
    inline float Clamp(float f, float min, float max) { return f < min ? min : f < max ? f : max; }

    inline float FloatSelect(float comparand, float valueGEZero, float valueLTZero)
    {
        return comparand >= 0.0f ? valueGEZero : valueLTZero;
    }

    inline float Lerp(float a, float b, float alpha) { return a + alpha * (b - a); }

    inline uint32_t MortonCode2(uint32_t x)
    {
        x &= 0x0000ffff;
        x = (x ^ (x << 8)) & 0x00ff00ff;
        x = (x ^ (x << 4)) & 0x0f0f0f0f;
        x = (x ^ (x << 2)) & 0x33333333;
        x = (x ^ (x << 1)) & 0x55555555;
        return x;
    }

    inline uint32_t ReverseMortonCode2(uint32_t x)
    {
        x &= 0x55555555;
        x = (x ^ (x >> 1)) & 0x33333333;
        x = (x ^ (x >> 2)) & 0x0f0f0f0f;
        x = (x ^ (x >> 4)) & 0x00ff00ff;
        x = (x ^ (x >> 8)) & 0x0000ffff;
        return x;
    }

    // Units in the last place of `value` (one ulp above it).
    inline double Ulpc(double value)
    {
        int64_t bits;
        std::memcpy(&bits, &value, sizeof(bits)); // BitConverter.DoubleToInt64Bits
        bits += 1;
        double next;
        std::memcpy(&next, &bits, sizeof(next));   // BitConverter.Int64BitsToDouble
        return next - value;
    }
}
