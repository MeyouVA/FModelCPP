// Ported from CUE4Parse/UE4/Wwise/Objects/Actions/CAkActionBypassFX.cs
#pragma once

#include <cstdint>

#include "../../WwiseArchive.h"
#include "CAkActionExcept.h"

namespace CUE4Parse::UE4::Wwise::Objects::Actions
{
    class CAkActionBypassFX
    {
    public:
        bool bIsBypass = false;
        uint8_t TargetMask = 0;
        uint8_t byFxSlot = 0;
        CAkActionExcept ExceptParams;

        CAkActionBypassFX() = default;

        // CAkActionBypassFX::SetActionParams
        explicit CAkActionBypassFX(FWwiseArchive& Ar)
        {
            bIsBypass = Ar.ReadBool();
            // Same byte, different meaning either side of 146; below 27 there is no byte at all.
            if (Ar.Version >= 146)
                byFxSlot = Ar.Read<uint8_t>();
            else if (Ar.Version >= 27)
                TargetMask = Ar.Read<uint8_t>();

            ExceptParams = CAkActionExcept(Ar);
        }
    };
}
