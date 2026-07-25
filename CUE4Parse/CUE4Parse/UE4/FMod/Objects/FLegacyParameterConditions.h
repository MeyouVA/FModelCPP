// Ported from CUE4Parse/UE4/FMod/Objects/FLegacyParameterConditions.cs
#pragma once

#include "FModGuid.h"

namespace CUE4Parse::UE4::FMod::Objects
{
    struct FLegacyParameterConditions
    {
        FModGuid BaseGuid;
        float Minimum = 0.0f;
        float Maximum = 0.0f;

        FLegacyParameterConditions() = default;
        explicit FLegacyParameterConditions(Readers::FArchive& Ar) : BaseGuid(Ar)
        {
            Minimum = Ar.Read<float>();
            Maximum = Ar.Read<float>();
        }
    };
}
