// Ported from CUE4Parse/UE4/Objects/Core/Math/UnrealMathUtility.cs (scalar subset).
// The tolerance constants + IsNearly* helpers (curve evaluation, CubicCurve2D) plus the Min3/Max3/Fmod scalar
// helpers used by the Core/Math color conversions. The full UnrealMath (vector/quat/SSE helpers) arrives with the
// rest of the Core/Math layer. C#'s `static class UnrealMath` becomes a namespace of inline free functions here.
#pragma once

#include <cmath>
#include <cstdint>

namespace CUE4Parse::UE4::Objects::Core::Math::UnrealMath
{
    constexpr float SmallNumber = 1e-8f;
    constexpr float KindaSmallNumber = 1e-4f;

    inline bool IsNearlyEqual(float a, float b, float err = SmallNumber) { return std::fabs(a - b) <= err; }
    inline bool IsNearlyZero(float x, float tolerance = KindaSmallNumber) { return std::fabs(x) <= tolerance; }
    inline bool IsNearlyZero(double x, double tolerance = KindaSmallNumber) { return std::abs(x) <= tolerance; }

    inline float Min3(float a, float b, float c) { float m = b < c ? b : c; return a < m ? a : m; }
    inline float Max3(float a, float b, float c) { float m = b > c ? b : c; return a > m ? a : m; }

    // UE's fmod: bounded, avoids the large-argument precision loss of std::fmod. Mirrors the C# implementation
    // (TruncToInt + Clamp inlined here to keep this header free of a MathUtils include cycle).
    inline float Fmod(float x, float y)
    {
        const float absY = std::fabs(y);
        if (absY <= SmallNumber) return 0.0f;
        const float div = x / y;
        const float quotient = std::fabs(div) < 8388608.0f ? static_cast<float>(static_cast<int32_t>(div)) : div;
        float intPortion = y * quotient;
        if (std::fabs(intPortion) > std::fabs(x))
            intPortion = x;
        const float res = x - intPortion;
        return res < -absY ? -absY : res < absY ? res : absY; // Clamp(res, -absY, absY)
    }
}
