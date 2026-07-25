// Ported from CUE4Parse/UE4/Wwise/Objects/Actions/CAkActionPause.cs
#pragma once

#include <cstdint>

#include "../../WwiseArchive.h"
#include "../../Enums/Flags/EPauseOptionsFlags.h"
#include "CAkActionExcept.h"
#include "CAkActionParams.h"

namespace CUE4Parse::UE4::Wwise::Objects::Actions
{
    using CUE4Parse::UE4::Wwise::Enums::Flags::EPauseOptionsFlags;

    class CAkActionPause
    {
    public:
        CAkActionParams ActionParams;
        EPauseOptionsFlags PauseOptions = static_cast<EPauseOptionsFlags>(0);
        CAkActionExcept ExceptParams;

        CAkActionPause() = default;

        // CAkActionPause::SetActionSpecificParams
        explicit CAkActionPause(FWwiseArchive& Ar)
        {
            ActionParams = CAkActionParams(Ar);

            if (Ar.Version <= 56)
                Ar.Read<uint32_t>(); // IsMaster
            else if (Ar.Version <= 62)
                Ar.Read<uint8_t>();  // IsMaster
            else
                PauseOptions = Ar.Read<EPauseOptionsFlags>();

            ExceptParams = CAkActionExcept(Ar);
        }
    };
}
