// Ported from CUE4Parse/UE4/FMod/Objects/FControllerOverride.cs
#pragma once

#include "FModGuid.h"

namespace CUE4Parse::UE4::FMod::Objects
{
    struct FControllerOverride
    {
        FModGuid ControllerGuid;
        FModGuid CurveGuid;

        FControllerOverride() = default;
        explicit FControllerOverride(Readers::FArchive& Ar)
            : ControllerGuid(Ar), CurveGuid(Ar) {}
    };
}
