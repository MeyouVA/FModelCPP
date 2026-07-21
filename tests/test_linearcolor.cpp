// Unit tests for the Core/Math color layer: UnrealMath scalar helpers (Min3/Max3/Fmod), FColor (Hex packing,
// ToPackedARGB, Requantize16to8) and FLinearColor (sRGB-off quantization to FColor, the HSV round-trip that
// LinearRGBToHsv/HSVToLinearRGB form, WithAlpha). No package machinery -- these types are pure value math.
#include <cmath>
#include <iostream>
#include <string>

#include "UE4/Objects/Core/Math/FColor.h"
#include "UE4/Objects/Core/Math/FLinearColor.h"
#include "UE4/Objects/Core/Math/UnrealMathUtility.h"

using CUE4Parse::UE4::Objects::Core::Math::FColor;
using CUE4Parse::UE4::Objects::Core::Math::FLinearColor;
namespace UM = CUE4Parse::UE4::Objects::Core::Math::UnrealMath;

static int g_failures = 0;
#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::cerr << "FAIL: " << #cond << " (line " << __LINE__ << ")\n";  \
            ++g_failures;                                                      \
        }                                                                      \
    } while (0)

static bool Near(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) < eps; }

int main()
{
    // ---------- UnrealMath scalar helpers ----------
    CHECK(Near(UM::Min3(2.0f, 0.5f, 10.0f), 0.5f));
    CHECK(Near(UM::Max3(2.0f, 0.5f, 10.0f), 10.0f));
    CHECK(Near(UM::Fmod(370.0f, 360.0f), 10.0f));   // small quotient path
    CHECK(Near(UM::Fmod(-40.0f, 360.0f), -40.0f));  // negative, |x| < |y|
    CHECK(Near(UM::Fmod(5.0f, 0.0f), 0.0f));        // |y| <= SmallNumber guard

    // ---------- FColor ----------
    CHECK(FColor(255, 128, 0).Hex() == "FF8000");           // opaque -> RGB only
    CHECK(FColor(255, 128, 0, 64).Hex() == "40FF8000");     // translucent -> ARGB
    CHECK(FColor(0x11, 0x22, 0x33, 0x44).ToPackedARGB() == 0x44112233);
    CHECK(FColor::Requantize16to8(65535) == 255);
    CHECK(FColor::Requantize16to8(0) == 0);
    CHECK(FColor::Requantize16to8(32768) == 128);

    // ---------- FLinearColor -> FColor (sRGB off is a plain clamp+quantize) ----------
    {
        const FColor c = FLinearColor(0.5f, 0.25f, 1.0f, 1.0f).ToFColor(false);
        CHECK(c.R == 127 && c.G == 63 && c.B == 255 && c.A == 255);
        CHECK(c.Hex() == "7F3FFF");
    }
    // Values outside [0,1] clamp before quantizing.
    {
        const FColor c = FLinearColor(2.0f, -1.0f, 0.0f, 0.5f).ToFColor(false);
        CHECK(c.R == 255 && c.G == 0 && c.B == 0);
        CHECK(c.A == 127); // 0.5 * 255.999 -> 127
    }

    // ---------- HSV round-trip: LinearRGBToHsv then HSVToLinearRGB reconstructs the RGB ----------
    {
        const FLinearColor orig(0.2f, 0.5f, 0.8f, 0.5f);
        const FLinearColor rgb = orig.LinearRGBToHsv().HSVToLinearRGB();
        CHECK(Near(rgb.R, 0.2f, 1e-3f));
        CHECK(Near(rgb.G, 0.5f, 1e-3f));
        CHECK(Near(rgb.B, 0.8f, 1e-3f));
        CHECK(Near(rgb.A, 0.5f)); // alpha carried through unchanged
    }
    // A second hue sector (max == R) to cover a different swizzle row.
    {
        const FLinearColor orig(0.9f, 0.3f, 0.1f, 1.0f);
        const FLinearColor rgb = orig.LinearRGBToHsv().HSVToLinearRGB();
        CHECK(Near(rgb.R, 0.9f, 1e-3f));
        CHECK(Near(rgb.G, 0.3f, 1e-3f));
        CHECK(Near(rgb.B, 0.1f, 1e-3f));
    }

    // ---------- WithAlpha ----------
    {
        const FLinearColor c = FLinearColor(0.1f, 0.2f, 0.3f, 1.0f).WithAlpha(0.42f);
        CHECK(Near(c.R, 0.1f) && Near(c.G, 0.2f) && Near(c.B, 0.3f) && Near(c.A, 0.42f));
    }

    if (g_failures == 0)
    {
        std::cout << "All linear-color tests passed.\n";
        return 0;
    }
    std::cout << g_failures << " check(s) failed.\n";
    return 1;
}
