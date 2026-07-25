// Ported from CUE4Parse/UE4/Wwise/Objects/Actions/CAkActionSeek.cs
#pragma once

#include <cstdint>

#include "../../WwiseArchive.h"
#include "AkRandomizerModifier.h"
#include "CAkActionExcept.h"

namespace CUE4Parse::UE4::Wwise::Objects::Actions
{
    class CAkActionSeek
    {
    public:
        uint8_t IsSeekRelativeToDuration = 0;
        AkRandomizerModifier RandomizerModifier;
        uint8_t SnapToNearestMarker = 0;
        CAkActionExcept ExceptParams;

        CAkActionSeek() = default;

        // CAkActionSeek::SetActionParams
        explicit CAkActionSeek(FWwiseArchive& Ar)
        {
            IsSeekRelativeToDuration = Ar.Read<uint8_t>();
            RandomizerModifier = AkRandomizerModifier(Ar);
            SnapToNearestMarker = Ar.Read<uint8_t>();
            ExceptParams = CAkActionExcept(Ar);
        }
    };
}
