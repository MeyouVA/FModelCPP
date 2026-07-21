// Ported from CUE4Parse/UE4/Objects/Core/Math/FLinearColor.cs
// A linear, 32-bit-per-component floating point RGBA color.
//
// Deliberate differences from C#:
//   * The implicit FLinearColor -> Vector4 / Vector3 conversions are omitted (System.Numerics; no consumer in the
//     port yet). TODO if a vector consumer arrives.
#pragma once

#include <string>

#include "FColor.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    struct FLinearColor
    {
        float R = 0.0f;
        float G = 0.0f;
        float B = 0.0f;
        float A = 0.0f;

        FLinearColor() = default;
        FLinearColor(float r, float g, float b, float a) : R(r), G(g), B(b), A(a) {}

        FColor ToFColor(bool sRGB) const;
        FLinearColor ToSRGB() const;

        std::string Hex() const { return ToFColor(true).Hex(); }
        std::string ToString() const { return Hex(); }

        FLinearColor WithAlpha(float alpha) const { return FLinearColor(R, G, B, alpha); }

        FLinearColor LinearRGBToHsv() const;
        FLinearColor HSVToLinearRGB() const;
    };

    inline const FLinearColor FLinearColor_Gray{0.6f, 0.6f, 0.6f, 1.0f};
}
