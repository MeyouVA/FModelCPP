// Ported from CUE4Parse/UE4/Wwise/Objects/Actions/CAkActionSetAkProp.cs
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

    class CAkActionSetAkProp
    {
    public:
        CAkActionParams ActionParams;
        EAkValueMeaning ValueMeaning = static_cast<EAkValueMeaning>(0);
        AkRandomizerModifier RandomizerModifier;
        CAkActionExcept ExceptParams;

        CAkActionSetAkProp() = default;

        // CAkActionSetAkProp::SetActionSpecificParams
        explicit CAkActionSetAkProp(FWwiseArchive& Ar)
        {
            ActionParams = CAkActionParams(Ar);

            if (Ar.Version <= 56)
                ValueMeaning = static_cast<EAkValueMeaning>(Ar.Read<uint32_t>());
            else
                ValueMeaning = static_cast<EAkValueMeaning>(Ar.Read<uint8_t>());

            RandomizerModifier = AkRandomizerModifier(Ar);
            ExceptParams = CAkActionExcept(Ar);
        }
    };
}
