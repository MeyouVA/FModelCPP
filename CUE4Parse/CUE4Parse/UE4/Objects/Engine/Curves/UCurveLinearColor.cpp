// Ported from CUE4Parse/UE4/Objects/Engine/Curves/UCurveLinearColor.cs
// (Deserialize + the GetUnadjustedLinearColorValue / GetLinearColorValue color accessors).
#include "UCurveLinearColor.h"

#include <cmath>
#include <cstddef>

#include "../../../Assets/Readers/FAssetArchive.h"
#include "../../../Assets/Objects/FPropertyTag.h"
#include "../../../Assets/Objects/FScriptStruct.h"
#include "../../../Assets/Objects/FStructFallback.h"
#include "../../../Assets/Objects/Properties/StructProperty.h"
#include "../../../Assets/Objects/Properties/FloatProperty.h"
#include "../../Core/Math/UnrealMathUtility.h"
#include "../../../../Utils/MathUtils.h"

namespace CUE4Parse::UE4::Objects::Engine::Curves
{
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;
    using CUE4Parse::UE4::Assets::Objects::FPropertyTag;
    using CUE4Parse::UE4::Assets::Objects::Properties::StructProperty;
    using CUE4Parse::UE4::Assets::Objects::Properties::FloatProperty;
    using CUE4Parse::UE4::Objects::Core::Math::FLinearColor;
    namespace UM = CUE4Parse::UE4::Objects::Core::Math::UnrealMath;

    namespace
    {
        // Stand-in for C#'s GetOrDefault<float>(name): scan the object's tagged properties for a FloatProperty.
        float GetFloatProp(const std::vector<FPropertyTag>& props, const char* name, float def = 0.0f)
        {
            for (const auto& tag : props)
            {
                if (tag.Name.Text() != name)
                    continue;
                if (const auto* fp = dynamic_cast<const FloatProperty*>(tag.Tag.get()))
                    return fp->Value;
                break;
            }
            return def;
        }
    }

    void UCurveLinearColor::Deserialize(FAssetArchive& Ar, int64_t validPos)
    {
        UCurveBase::Deserialize(Ar, validPos);

        AdjustBrightness = GetFloatProp(Properties, "AdjustBrightness");
        AdjustBrightnessCurve = GetFloatProp(Properties, "AdjustBrightnessCurve");
        AdjustVibrance = GetFloatProp(Properties, "AdjustVibrance");
        AdjustSaturation = GetFloatProp(Properties, "AdjustSaturation");
        AdjustHue = GetFloatProp(Properties, "AdjustHue");
        AdjustMinAlpha = GetFloatProp(Properties, "AdjustMinAlpha");
        AdjustMaxAlpha = GetFloatProp(Properties, "AdjustMaxAlpha");

        for (std::size_t i = 0; i < Properties.size() && i < FloatCurves.size(); ++i)
        {
            if (const auto* sp = dynamic_cast<const StructProperty*>(Properties[i].Tag.get()))
            {
                if (sp->Value.Struct)
                    FloatCurves[i] = FRichCurve(*sp->Value.Struct);
            }
        }
    }

    FLinearColor UCurveLinearColor::GetUnadjustedLinearColorValue(float inTime) const
    {
        return FLinearColor(FloatCurves[0].Eval(inTime), FloatCurves[1].Eval(inTime), FloatCurves[2].Eval(inTime),
                            FloatCurves[3].Keys.empty() ? 1.0f : FloatCurves[3].Eval(inTime));
    }

    FLinearColor UCurveLinearColor::GetLinearColorValue(float inTime) const
    {
        const FLinearColor originalColor = GetUnadjustedLinearColorValue(inTime);

        const bool bShouldClampValue = originalColor.R <= 1.0f && originalColor.G <= 1.0f && originalColor.B <= 1.0f;

        const FLinearColor hsvColor = originalColor.LinearRGBToHsv();
        float pixelHue = hsvColor.R;
        float pixelSaturation = hsvColor.G;
        float pixelValue = hsvColor.B;

        pixelValue *= AdjustBrightness;

        if (!UM::IsNearlyEqual(AdjustBrightnessCurve, 1.0f, UM::KindaSmallNumber) && AdjustBrightnessCurve != 0.0f)
        {
            // Raise HSV.V to the specified power
            pixelValue = static_cast<float>(std::pow(pixelValue, AdjustBrightnessCurve));
        }

        // Apply "vibrancy" adjustment
        if (!UM::IsNearlyZero(AdjustBrightness))
        {
            const double invSatRaised = std::pow(1.0f - pixelSaturation, 5.0f);
            const float clampedVibrance = CUE4Parse::Utils::Clamp(AdjustVibrance, 0.0f, 1.0f);
            const float halfVibrance = clampedVibrance * 0.5f;
            const double satProduct = halfVibrance * invSatRaised;

            pixelSaturation += static_cast<float>(satProduct);
        }

        // Apply saturation adjustment
        pixelSaturation *= AdjustSaturation;

        // Apply hue adjustment
        pixelHue += AdjustHue;

        // Clamp HSV values
        {
            pixelHue = UM::Fmod(pixelHue, 360.0f);
            if (pixelHue < 0.0f)
            {
                // Keep the hue value positive as HSVToLinearRGB prefers that
                pixelHue += 360.0f;
            }

            pixelSaturation = CUE4Parse::Utils::Clamp(pixelSaturation, 0.0f, 1.0f);

            if (bShouldClampValue)
            {
                pixelValue = CUE4Parse::Utils::Clamp(pixelValue, 0.0f, 1.0f);
            }
        }

        const FLinearColor linearColor = hsvColor.HSVToLinearRGB();
        const float newAlpha = CUE4Parse::Utils::Lerp(AdjustMinAlpha, AdjustMaxAlpha, originalColor.A);
        return linearColor.WithAlpha(newAlpha);
    }
}
