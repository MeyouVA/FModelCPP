// Ported from CUE4Parse/UE4/Wwise/Objects/HIRC/Containers/HierarchyMusicSegment.cs
#pragma once

#include <vector>

#include "../../../WwiseArchive.h"
#include "../../AkMeterInfo.h"
#include "../../AkMusicMarkerWwise.h"
#include "../../AkStinger.h"
#include "../BaseHierarchyMusic.h"

namespace CUE4Parse::UE4::Wwise::Objects::HIRC::Containers
{
    // CAkMusicSegment
    class HierarchyMusicSegment : public BaseHierarchyMusic
    {
    public:
        AkMeterInfo MeterInfo;
        std::vector<AkStinger> Stingers;
        double Duration = 0;
        std::vector<AkMusicMarkerWwise> Markers;

        // CAkMusicSegment::SetInitialValues
        explicit HierarchyMusicSegment(FWwiseArchive& Ar) : BaseHierarchyMusic(Ar)
        {
            MeterInfo = AkMeterInfo(Ar);
            Stingers = AkStinger::ReadArray(Ar);
            Duration = Ar.Read<double>();
            Markers = AkMusicMarkerWwise::ReadArray(Ar);
        }
    };
}
