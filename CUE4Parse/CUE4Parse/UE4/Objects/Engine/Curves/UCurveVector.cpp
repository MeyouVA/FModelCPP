// Ported from CUE4Parse/UE4/Objects/Engine/Curves/UCurveVector.cs.
#include "UCurveVector.h"

#include <cstddef>

#include "../../../Assets/Readers/FAssetArchive.h"
#include "../../../Assets/Objects/FPropertyTag.h"
#include "../../../Assets/Objects/FScriptStruct.h"
#include "../../../Assets/Objects/FStructFallback.h"
#include "../../../Assets/Objects/Properties/StructProperty.h"

namespace CUE4Parse::UE4::Objects::Engine::Curves
{
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;
    using CUE4Parse::UE4::Assets::Objects::Properties::StructProperty;

    void UCurveVector::Deserialize(FAssetArchive& Ar, int64_t validPos)
    {
        UCurveBase::Deserialize(Ar, validPos);

        // C#: for each property whose value is FScriptStruct { StructType: FStructFallback }, build FloatCurves[i].
        // The index is positional over Properties; bounded to the array size (see header note).
        for (std::size_t i = 0; i < Properties.size() && i < FloatCurves.size(); ++i)
        {
            if (const auto* sp = dynamic_cast<const StructProperty*>(Properties[i].Tag.get()))
            {
                if (const auto* fallback = sp->Value.AsFallback())
                    FloatCurves[i] = FRichCurve(*fallback);
            }
        }
    }
}
