// Ported from CUE4Parse/UE4/Wwise/Objects/AkStateTransition.cs
#pragma once

#include <cstdint>

#include "../WwiseArchive.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    struct AkStateTransition
    {
        uint32_t StateFrom = 0;
        uint32_t StateTo = 0;
        uint32_t TransitionTime = 0;

        AkStateTransition() = default;

        explicit AkStateTransition(FWwiseArchive& Ar)
        {
            StateFrom = Ar.Read<uint32_t>();
            StateTo = Ar.Read<uint32_t>();
            TransitionTime = Ar.Read<uint32_t>();
        }
    };
}
