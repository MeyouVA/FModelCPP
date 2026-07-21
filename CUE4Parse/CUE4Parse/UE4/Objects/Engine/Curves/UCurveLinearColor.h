// Ported from CUE4Parse/UE4/Objects/Engine/Curves/UCurveLinearColor.cs
// A curve-linear-color asset: four FRichCurves (R/G/B/A) plus a set of HSV adjustment scalars used by the
// GetLinearColorValue helper (brightness/vibrance/saturation/hue/alpha remap).
//
// Deliberate differences from C#:
//   * FloatCurves is a std::array<FRichCurve, 4> of default-constructed curves (see the UCurveVector note on the
//     null-vs-empty and positional-index bounds).
//   * GetUnadjustedLinearColorValue / GetLinearColorValue are DEFERRED: they return an FLinearColor and use its
//     LinearRGBToHsv / HSVToLinearRGB / WithAlpha color-space conversions, which live in the not-yet-ported
//     Core/Math layer (FLinearColor). The Adjust* scalars are still deserialized so the data is complete; the two
//     accessors arrive with FLinearColor. TODO.
//   * WriteJson is omitted.
#pragma once

#include <array>
#include <cstdint>

#include "RichCurve.h"
#include "UCurveBase.h"

namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }

namespace CUE4Parse::UE4::Objects::Engine::Curves
{
    class UCurveLinearColor : public UCurveBase
    {
    public:
        std::array<FRichCurve, 4> FloatCurves;

        void Deserialize(CUE4Parse::UE4::Assets::Readers::FAssetArchive& Ar, int64_t validPos) override;

        // GetUnadjustedLinearColorValue / GetLinearColorValue deferred until FLinearColor is ported (see header note).

    private:
        // HSV adjustment scalars (read from the object's tagged properties; consumed by the deferred accessors).
        float AdjustBrightness = 0.0f;
        float AdjustBrightnessCurve = 0.0f;
        float AdjustVibrance = 0.0f;
        float AdjustSaturation = 0.0f;
        float AdjustHue = 0.0f;
        float AdjustMinAlpha = 0.0f;
        float AdjustMaxAlpha = 0.0f;
    };
}
