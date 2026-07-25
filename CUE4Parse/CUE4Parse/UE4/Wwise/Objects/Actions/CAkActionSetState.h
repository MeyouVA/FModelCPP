// Ported from CUE4Parse/UE4/Wwise/Objects/Actions/CAkActionSetState.cs
#pragma once

#include <cstdint>

#include "../../WwiseArchive.h"

namespace CUE4Parse::UE4::Wwise::Objects::Actions
{
    class CAkActionSetState
    {
    public:
        uint32_t StateGroupId = 0;
        uint32_t TargetStateId = 0;

        CAkActionSetState() = default;

        explicit CAkActionSetState(FWwiseArchive& Ar)
        {
            StateGroupId = Ar.Read<uint32_t>();
            TargetStateId = Ar.Read<uint32_t>();
        }
    };
}
