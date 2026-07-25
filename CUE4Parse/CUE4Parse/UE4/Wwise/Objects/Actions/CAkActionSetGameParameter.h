// Ported from CUE4Parse/UE4/Wwise/Objects/Actions/CAkActionSetGameParameter.cs
#pragma once

#include <cstdint>

#include "../../WwiseArchive.h"
#include "../../Enums/EAkValueMeaning.h"
#include "AkRandomizerModifier.h"
#include "CAkActionExcept.h"
#include "CAkActionParams.h"

namespace CUE4Parse::UE4::Wwise::Objects::Actions
{
    using CUE4Parse::UE4::Wwise::Enums::EAkValueMeaning;

    class CAkActionSetGameParameter
    {
    public:
        CAkActionParams ActionParams;
        bool BypassTransition = false;
        EAkValueMeaning ValueMeaning = static_cast<EAkValueMeaning>(0);
        AkRandomizerModifier RandomizerModifier;
        CAkActionExcept ExceptParams;

        CAkActionSetGameParameter() = default;

        // CAkActionSetGameParameter::SetActionSpecificParams
        explicit CAkActionSetGameParameter(FWwiseArchive& Ar)
        {
            ActionParams = CAkActionParams(Ar);
            if (Ar.Version > 89)
            {
                BypassTransition = Ar.ReadBool();
            }

            if (Ar.Version <= 56)
                ValueMeaning = static_cast<EAkValueMeaning>(Ar.Read<uint32_t>());
            else
                ValueMeaning = static_cast<EAkValueMeaning>(Ar.Read<uint8_t>());

            RandomizerModifier = AkRandomizerModifier(Ar);
            ExceptParams = CAkActionExcept(Ar);
        }
    };
}
