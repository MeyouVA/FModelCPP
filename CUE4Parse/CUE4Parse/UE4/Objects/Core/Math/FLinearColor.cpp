// Ported from CUE4Parse/UE4/Objects/Core/Math/FLinearColor.cs (the sRGB/HSV conversion bodies).
#include "FLinearColor.h"

#include <cmath>
#include <cstdint>

#include "UnrealMathUtility.h"
#include "../../../../Utils/MathUtils.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    using CUE4Parse::Utils::Clamp;
    using CUE4Parse::Utils::FloorToInt;
    namespace UM = CUE4Parse::UE4::Objects::Core::Math::UnrealMath;

    FColor FLinearColor::ToFColor(bool sRGB) const
    {
        float floatR = Clamp(R, 0.0f, 1.0f);
        float floatG = Clamp(G, 0.0f, 1.0f);
        float floatB = Clamp(B, 0.0f, 1.0f);
        float floatA = Clamp(A, 0.0f, 1.0f);

        if (sRGB)
        {
            floatR = floatR <= 0.0031308f ? floatR * 12.92f : std::pow(floatR, 1.0f / 2.4f) * 1.055f - 0.055f;
            floatG = floatG <= 0.0031308f ? floatG * 12.92f : std::pow(floatG, 1.0f / 2.4f) * 1.055f - 0.055f;
            floatB = floatB <= 0.0031308f ? floatB * 12.92f : std::pow(floatB, 1.0f / 2.4f) * 1.055f - 0.055f;
        }

        const int32_t intA = FloorToInt(floatA * 255.999f);
        const int32_t intR = FloorToInt(floatR * 255.999f);
        const int32_t intG = FloorToInt(floatG * 255.999f);
        const int32_t intB = FloorToInt(floatB * 255.999f);

        return FColor(static_cast<uint8_t>(intR), static_cast<uint8_t>(intG),
                      static_cast<uint8_t>(intB), static_cast<uint8_t>(intA));
    }

    FLinearColor FLinearColor::ToSRGB() const
    {
        float floatR = Clamp(R, 0.0f, 1.0f);
        float floatG = Clamp(G, 0.0f, 1.0f);
        float floatB = Clamp(B, 0.0f, 1.0f);

        floatR = floatR <= 0.0031308f ? floatR * 12.92f : std::pow(floatR, 1.0f / 2.4f) * 1.055f - 0.055f;
        floatG = floatG <= 0.0031308f ? floatG * 12.92f : std::pow(floatG, 1.0f / 2.4f) * 1.055f - 0.055f;
        floatB = floatB <= 0.0031308f ? floatB * 12.92f : std::pow(floatB, 1.0f / 2.4f) * 1.055f - 0.055f;

        return FLinearColor(floatR, floatG, floatB, A);
    }

    FLinearColor FLinearColor::LinearRGBToHsv() const
    {
        const float rgbMin = UM::Min3(R, G, B);
        const float rgbMax = UM::Max3(R, G, B);
        const float rgbRange = rgbMax - rgbMin;

        const float hue = rgbMax == rgbMin ? 0.0f :
            rgbMax == R ? UM::Fmod((G - B) / rgbRange * 60.0f + 360.0f, 360.0f) :
            rgbMax == G ? (B - R) / rgbRange * 60.0f + 120.0f :
            rgbMax == B ? (R - G) / rgbRange * 60.0f + 240.0f :
            0.0f;

        const float saturation = rgbMax == 0.0f ? 0.0f : rgbRange / rgbMax;
        return FLinearColor(hue, saturation, rgbMax, A);
    }

    FLinearColor FLinearColor::HSVToLinearRGB() const
    {
        const float hue = R;
        const float saturation = G;
        const float value = B;
        const float hDiv60 = hue / 60.0f;
        const float hDiv60Floor = std::floor(hDiv60);
        const float hDiv60Fraction = hDiv60 - hDiv60Floor;

        const float rgbValues[4] = {
            value,
            value * (1.0f - saturation),
            value * (1.0f - hDiv60Fraction * saturation),
            value * (1.0f - (1.0f - hDiv60Fraction) * saturation)
        };
        static constexpr uint32_t rgbSwizzle[6][3] = {
            {0, 3, 1},
            {2, 0, 1},
            {1, 0, 3},
            {1, 2, 0},
            {3, 1, 0},
            {0, 1, 2}
        };

        const uint32_t swizzleIndex = static_cast<uint32_t>(hDiv60Floor) % 6;

        return FLinearColor(rgbValues[rgbSwizzle[swizzleIndex][0]],
                            rgbValues[rgbSwizzle[swizzleIndex][1]],
                            rgbValues[rgbSwizzle[swizzleIndex][2]],
                            A);
    }
}
