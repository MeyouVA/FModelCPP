// Ported from CUE4Parse/UE4/FMod/Objects/FRangeFloat.cs
#pragma once

#include "../../Readers/FArchive.h"

namespace CUE4Parse::UE4::FMod::Objects
{
    struct FRangeFloat
    {
        float Minimum = 0.0f;
        float Maximum = 0.0f;

        FRangeFloat() = default;
        explicit FRangeFloat(Readers::FArchive& Ar)
        {
            Minimum = Ar.Read<float>();
            Maximum = Ar.Read<float>();
        }
    };
}
