// Ported from CUE4Parse/UE4/FMod/Objects/FCurvePoint.cs
#pragma once

#include "../../Readers/FArchive.h"

namespace CUE4Parse::UE4::FMod::Objects
{
    struct FCurvePoint
    {
        float X = 0.0f;
        float Y = 0.0f;
        float Shape = 0.0f;
        uint32_t Type = 0;

        FCurvePoint() = default;
        explicit FCurvePoint(Readers::FArchive& Ar)
        {
            X = Ar.Read<float>();
            Y = Ar.Read<float>();
            Shape = Ar.Read<float>();
            Type = Ar.Read<uint32_t>();
        }
    };
}
