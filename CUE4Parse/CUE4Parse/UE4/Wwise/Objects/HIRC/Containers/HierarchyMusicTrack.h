// Ported from CUE4Parse/UE4/Wwise/Objects/HIRC/Containers/HierarchyMusicTrack.cs
#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "../../../WwiseArchive.h"
#include "../../../Enums/EAkMusicTrackType.h"
#include "../../../Enums/Flags/EMusicFlags.h"
#include "../../AkBankSourceData.h"
#include "../../AkClipAutomation.h"
#include "../../AkMusicTrackTransRule.h"
#include "../../AkTrackSrcInfo.h"
#include "../../AkTrackSwitchParams.h"
#include "../AbstractHierarchy.h"
#include "../BaseHierarchy.h"

namespace CUE4Parse::UE4::Wwise::Objects::HIRC::Containers
{
    using CUE4Parse::UE4::Wwise::Enums::EAkMusicTrackType;
    using CUE4Parse::UE4::Wwise::Enums::Flags::EMusicFlags;

    class HierarchyMusicTrack : public AbstractHierarchy
    {
    public:
        EMusicFlags MusicFlags = static_cast<EMusicFlags>(0);
        std::vector<AkBankSourceData> Sources;
        std::vector<AkTrackSrcInfo> Playlist;
        std::vector<AkClipAutomation> ClipAutomations;
        BaseHierarchy BaseParams;
        int16_t Loop = 0;
        int16_t LoopModMin = 0;
        int16_t LoopModMax = 0;
        uint32_t ERSType = 0;
        EAkMusicTrackType MusicTrackType = static_cast<EAkMusicTrackType>(0);
        std::optional<AkTrackSwitchParams> SwitchParams;
        std::optional<AkMusicTrackTransRule> TransParams;
        int32_t LookAheadTime = 0;

        // CAkMusicTrack::SetInitialValues
        explicit HierarchyMusicTrack(FWwiseArchive& Ar)
        {
            Id = Ar.Read<uint32_t>();
            // The music flags moved: present before the sources for 90..152, after them past 152, and
            // absent entirely at or below 89. C# writes two identical arms for the 90..112 and 113..152
            // ranges; folded to one condition here.
            if (Ar.Version > 89 && Ar.Version <= 152)
            {
                MusicFlags = Ar.Read<EMusicFlags>();
            }

            const int numSources = static_cast<int>(Ar.Read<uint32_t>());
            if (Ar.Version <= 26)
            {
                Ar.ReadArray<uint32_t>(numSources); // DataIndexes -- read and dropped, as in C#
            }

            Sources.reserve(static_cast<size_t>(numSources));
            for (int i = 0; i < numSources; i++)
                Sources.emplace_back(Ar);

            if (Ar.Version > 152)
            {
                MusicFlags = Ar.Read<EMusicFlags>();
            }

            if (Ar.Version > 26)
            {
                const uint32_t numPlaylistItems = Ar.Read<uint32_t>();
                if (numPlaylistItems > 0)
                {
                    Playlist = Ar.ReadArrayWith(static_cast<int>(numPlaylistItems),
                                                [&Ar] { return AkTrackSrcInfo(Ar); });
                    Ar.Read<uint32_t>(); // numSubTrack -- only present when the playlist is non-empty
                }
            }

            if (Ar.Version > 62)
            {
                ClipAutomations = Ar.ReadArrayWith([&Ar] { return AkClipAutomation(Ar); });
            }

            BaseParams = BaseHierarchy(Ar);

            if (Ar.Version <= 56)
            {
                Loop = Ar.Read<int16_t>();
                LoopModMin = Ar.Read<int16_t>();
                LoopModMax = Ar.Read<int16_t>();
            }

            if (Ar.Version <= 89)
            {
                ERSType = Ar.Read<uint32_t>();
            }
            else
            {
                MusicTrackType = Ar.Read<EAkMusicTrackType>();
                if (MusicTrackType == EAkMusicTrackType::Switch) // Special case for track type
                {
                    SwitchParams = AkTrackSwitchParams(Ar);
                    TransParams = AkMusicTrackTransRule(Ar);
                }
            }

            LookAheadTime = Ar.Read<int32_t>();

            // The oldest banks put the playlist at the *end* instead, after the look-ahead time.
            if (Ar.Version <= 26)
            {
                const uint32_t numPlaylistItems = Ar.Read<uint32_t>();
                if (numPlaylistItems > 0)
                {
                    Playlist = Ar.ReadArrayWith(static_cast<int>(numPlaylistItems),
                                                [&Ar] { return AkTrackSrcInfo(Ar); });
                }

                Ar.Read<uint32_t>(); // Unknown flag
            }
        }
    };
}
