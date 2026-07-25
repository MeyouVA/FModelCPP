// Ported from CUE4Parse/UE4/FMod/Objects/FFadeCurve.cs
#pragma once

#include "FModGuid.h"

namespace CUE4Parse::UE4::FMod::Objects
{
    struct FFadeCurve
    {
        FModGuid BusGuid;
        FModGuid CurveGuid;

        FFadeCurve() = default;
        explicit FFadeCurve(Readers::FArchive& Ar) : BusGuid(Ar), CurveGuid(Ar) {}
    };
}
