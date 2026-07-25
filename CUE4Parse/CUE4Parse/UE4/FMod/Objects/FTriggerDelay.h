// Ported from CUE4Parse/UE4/FMod/Objects/FTriggerDelay.cs
#pragma once

#include "../../Readers/FArchive.h"

namespace CUE4Parse::UE4::FMod::Objects
{
    struct FTriggerDelay
    {
        float Min = 0.0f;
        float Max = 0.0f;

        FTriggerDelay() = default;
        explicit FTriggerDelay(Readers::FArchive& Ar)
        {
            Min = Ar.Read<float>();
            Max = Ar.Read<float>();
        }
    };
}
