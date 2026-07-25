// Ported from CUE4Parse/UE4/FMod/Objects/FMappingPoint.cs
#pragma once

#include "../../Readers/FArchive.h"

namespace CUE4Parse::UE4::FMod::Objects
{
    struct FMappingPoint
    {
        float X = 0.0f;
        float Y = 0.0f;

        FMappingPoint() = default;
        explicit FMappingPoint(Readers::FArchive& Ar)
        {
            X = Ar.Read<float>();
            Y = Ar.Read<float>();
        }
    };
}
