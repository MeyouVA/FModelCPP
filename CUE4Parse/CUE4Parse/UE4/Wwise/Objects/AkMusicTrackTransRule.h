// Ported from CUE4Parse/UE4/Wwise/Objects/AkMusicTrackTransRule.cs
#pragma once

#include <cstdint>

#include "../WwiseArchive.h"
#include "../Enums/EAkSyncType.h"
#include "AkMusicFade.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    using CUE4Parse::UE4::Wwise::Enums::EAkSyncType;

    struct AkMusicTrackTransRule
    {
        AkMusicFade SourceFadeParams;
        EAkSyncType SyncType = static_cast<EAkSyncType>(0);
        uint32_t CueFilterHash = 0;
        AkMusicFade DestinationFadeParams;

        AkMusicTrackTransRule() = default;

        explicit AkMusicTrackTransRule(FWwiseArchive& Ar)
        {
            SourceFadeParams = AkMusicFade(Ar);
            SyncType = Ar.Read<EAkSyncType>();
            CueFilterHash = Ar.Read<uint32_t>();
            DestinationFadeParams = AkMusicFade(Ar);
        }
    };
}
