// Ported from CUE4Parse/UE4/Wwise/Objects/HIRC/Containers/HierarchyMusicRandomSequenceContainer.cs
#pragma once

#include <vector>

#include "../../../WwiseArchive.h"
#include "../../AkMeterInfo.h"
#include "../../AkMusicRanSeqPlaylistItem.h"
#include "../../AkMusicTransitionRule.h"
#include "../../AkStinger.h"
#include "../BaseHierarchyMusic.h"

namespace CUE4Parse::UE4::Wwise::Objects::HIRC::Containers
{
    class HierarchyMusicRandomSequenceContainer : public BaseHierarchyMusic
    {
    public:
        AkMeterInfo MeterInfo;
        std::vector<AkStinger> Stingers;
        AkMusicTransitionRule MusicTransitionRule;
        std::vector<AkMusicRanSeqPlaylistItem> Playlist;

        // CAkBankMgr::StdBankRead<CAkMusicRanSeqCntr>
        explicit HierarchyMusicRandomSequenceContainer(FWwiseArchive& Ar) : BaseHierarchyMusic(Ar)
        {
            MeterInfo = AkMeterInfo(Ar);
            Stingers = AkStinger::ReadArray(Ar);
            MusicTransitionRule = AkMusicTransitionRule(Ar);

            Ar.Read<uint32_t>(); // numPlaylistItems, I assume this is for parent and children together, therefore parent is always 1
            constexpr int numPlaylistItems = 1;
            Playlist = Ar.ReadArrayWith(numPlaylistItems, [&Ar] { return AkMusicRanSeqPlaylistItem(Ar); });
        }
    };
}
