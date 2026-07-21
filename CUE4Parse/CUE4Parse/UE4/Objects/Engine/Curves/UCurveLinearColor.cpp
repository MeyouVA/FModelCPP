// Ported from CUE4Parse/UE4/Objects/Engine/Curves/UCurveLinearColor.cs (Deserialize only; the color-value
// accessors are deferred with FLinearColor -- see the header note).
#include "UCurveLinearColor.h"

#include <cstddef>

#include "../../../Assets/Readers/FAssetArchive.h"
#include "../../../Assets/Objects/FPropertyTag.h"
#include "../../../Assets/Objects/FScriptStruct.h"
#include "../../../Assets/Objects/FStructFallback.h"
#include "../../../Assets/Objects/Properties/StructProperty.h"
#include "../../../Assets/Objects/Properties/FloatProperty.h"

namespace CUE4Parse::UE4::Objects::Engine::Curves
{
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;
    using CUE4Parse::UE4::Assets::Objects::FPropertyTag;
    using CUE4Parse::UE4::Assets::Objects::Properties::StructProperty;
    using CUE4Parse::UE4::Assets::Objects::Properties::FloatProperty;

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
}
