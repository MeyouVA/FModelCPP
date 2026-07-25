// Ported from CUE4Parse/UE4/Wwise/Objects/HIRC/Containers/HierarchyRandomSequenceContainer.cs
#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "../../../WwiseArchive.h"
#include "../../../Enums/EAkContainerMode.h"
#include "../../../Enums/EAkRandomMode.h"
#include "../../../Enums/EAkTransitionMode.h"
#include "../../../Enums/Flags/EPlaylistFlags.h"
#include "../../AkChildren.h"
#include "../../AkPlayList.h"
#include "../AbstractHierarchy.h"
#include "../BaseHierarchy.h"

namespace CUE4Parse::UE4::Wwise::Objects::HIRC::Containers
{
    using CUE4Parse::UE4::Wwise::Enums::EAkContainerMode;
    using CUE4Parse::UE4::Wwise::Enums::EAkRandomMode;
    using CUE4Parse::UE4::Wwise::Enums::EAkTransitionMode;
    // Note the enum is spelled EPlayListFlags (capital L) even though its file is EPlaylistFlags.cs.
    using CUE4Parse::UE4::Wwise::Enums::Flags::EPlayListFlags;

    class HierarchyRandomSequenceContainer : public AbstractHierarchy
    {
    public:
        BaseHierarchy BaseParams;
        uint16_t LoopCount = 0;
        std::optional<uint16_t> LoopModMin;
        std::optional<uint16_t> LoopModMax;
        std::optional<float> TransitionTime;
        std::optional<float> TransitionTimeModMin;
        std::optional<float> TransitionTimeModMax;
        uint16_t AvoidRepeatCount = 0;
        EAkTransitionMode TransitionMode = static_cast<EAkTransitionMode>(0);
        EAkRandomMode RandomMode = static_cast<EAkRandomMode>(0);
        EAkContainerMode Mode = static_cast<EAkContainerMode>(0);
        EPlayListFlags PlaylistFlags = EPlayListFlags::None;
        std::vector<uint32_t> ChildIds;
        std::vector<AkPlayListItem> Playlist;

        // CAkRanSeqCntr::SetInitialValues
        explicit HierarchyRandomSequenceContainer(FWwiseArchive& Ar)
        {
            Id = Ar.Read<uint32_t>();
            BaseParams = BaseHierarchy(Ar);
            LoopCount = Ar.Read<uint16_t>();

            if (Ar.Version > 72)
            {
                LoopModMin = Ar.Read<uint16_t>();
                LoopModMax = Ar.Read<uint16_t>();
            }

            // The transition times were ints before 39 and floats after; C# assigns both to float? so
            // the old values are converted, not reinterpreted.
            if (Ar.Version <= 38)
            {
                TransitionTime = static_cast<float>(Ar.Read<int32_t>());
                TransitionTimeModMin = static_cast<float>(Ar.Read<int32_t>());
                TransitionTimeModMax = static_cast<float>(Ar.Read<int32_t>());
            }
            else
            {
                TransitionTime = Ar.Read<float>();
                TransitionTimeModMin = Ar.Read<float>();
                TransitionTimeModMax = Ar.Read<float>();
            }

            AvoidRepeatCount = Ar.Read<uint16_t>();

            if (Ar.Version > 36)
            {
                TransitionMode = Ar.Read<EAkTransitionMode>();
                RandomMode = Ar.Read<EAkRandomMode>();
                Mode = Ar.Read<EAkContainerMode>();
            }

            // Five separate bools before 90, one packed flag byte after.
            if (Ar.Version <= 89)
            {
                PlaylistFlags = EPlayListFlags::None;
                if (Ar.ReadBool()) PlaylistFlags |= EPlayListFlags::IsUsingWeight;
                if (Ar.ReadBool()) PlaylistFlags |= EPlayListFlags::ResetPlayListAtEachPlay;
                if (Ar.ReadBool()) PlaylistFlags |= EPlayListFlags::IsRestartBackward;
                if (Ar.ReadBool()) PlaylistFlags |= EPlayListFlags::IsContinuous;
                if (Ar.ReadBool()) PlaylistFlags |= EPlayListFlags::IsGlobal;
            }
            else
            {
                PlaylistFlags = Ar.Read<EPlayListFlags>();
            }

            ChildIds = AkChildren(Ar).ChildIds;
            Playlist = AkPlayList(Ar).PlaylistItems;
        }
    };
}
