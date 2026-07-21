// Ported from CUE4Parse/UE4/Objects/Core/Math/UnrealMathUtility.cs (minimal subset).
// Only the scalar tolerance constants + the IsNearly* helpers used so far (curve evaluation, CubicCurve2D) are
// ported. The full UnrealMath (vector/quat/SSE helpers) arrives with the Core/Math layer. C#'s `static class
// UnrealMath` becomes a namespace of inline free functions/constants here.
#pragma once

#include <cmath>

namespace CUE4Parse::UE4::Objects::Core::Math::UnrealMath
{
    constexpr float SmallNumber = 1e-8f;
    constexpr float KindaSmallNumber = 1e-4f;

    inline bool IsNearlyEqual(float a, float b, float err = SmallNumber) { return std::fabs(a - b) <= err; }
    inline bool IsNearlyZero(float x, float tolerance = KindaSmallNumber) { return std::fabs(x) <= tolerance; }
    inline bool IsNearlyZero(double x, double tolerance = KindaSmallNumber) { return std::abs(x) <= tolerance; }
}
