// Ported from CUE4Parse/UE4/Wwise/Objects/Actions/CAkActionStop.cs
#pragma once

#include <cstdint>

#include "../../WwiseArchive.h"
#include "CAkActionExcept.h"
#include "CAkActionParams.h"

namespace CUE4Parse::UE4::Wwise::Objects::Actions
{
    class CAkActionStop
    {
    public:
        CAkActionParams ActionParams;
        bool ApplyToStateTransitions = false;
        bool ApplyToDynamicSequence = false;
        CAkActionExcept ExceptParams;

        CAkActionStop() = default;

        // CAkActionStop::SetActionActiveParams
        explicit CAkActionStop(FWwiseArchive& Ar)
        {
            ActionParams = CAkActionParams(Ar);
            if (Ar.Version > 122)
            {
                const uint8_t byBitVector = Ar.Read<uint8_t>();
                ApplyToStateTransitions = (byBitVector & (1 << 1)) != 0; // bit 1
                ApplyToDynamicSequence = (byBitVector & (1 << 2)) != 0;  // bit 2
            }

            ExceptParams = CAkActionExcept(Ar);
        }
    };
}
