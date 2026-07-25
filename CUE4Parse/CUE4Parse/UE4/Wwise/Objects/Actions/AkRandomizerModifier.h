// Ported from CUE4Parse/UE4/Wwise/Objects/Actions/AkRandomizerModifier.cs
#pragma once

#include "../../WwiseArchive.h"

namespace CUE4Parse::UE4::Wwise::Objects::Actions
{
    // Doesn't actually exist in Wwise but it's reused, RANGED_PARAMETER<float>
    struct AkRandomizerModifier
    {
        float Base = 0;
        float Min = 0;
        float Max = 0;

        AkRandomizerModifier() = default;

        explicit AkRandomizerModifier(FWwiseArchive& Ar)
        {
            Base = Ar.Read<float>();
            Min = Ar.Read<float>();
            Max = Ar.Read<float>();
        }
    };
}
