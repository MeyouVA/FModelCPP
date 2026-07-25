// Ported from CUE4Parse/UE4/Wwise/Objects/Actions/CAkActionSetSwitch.cs
#pragma once

#include <cstdint>

#include "../../WwiseArchive.h"

namespace CUE4Parse::UE4::Wwise::Objects::Actions
{
    struct CAkActionSetSwitch
    {
        uint32_t SwitchGroupId = 0;
        uint32_t SwitchStateId = 0;

        CAkActionSetSwitch() = default;

        // CAkActionSetSwitch::SetActionParams
        explicit CAkActionSetSwitch(FWwiseArchive& Ar)
        {
            SwitchGroupId = Ar.Read<uint32_t>();
            SwitchStateId = Ar.Read<uint32_t>();
        }
    };
}
