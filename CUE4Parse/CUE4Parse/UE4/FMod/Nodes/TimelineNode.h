// Ported from CUE4Parse/UE4/FMod/Nodes/TimelineNode.cs
#pragma once

#include <vector>

#include "../Objects/FModGuid.h"
#include "../Objects/FTriggerBox.h"
#include "../Objects/FSustainPoint.h"
#include "../Objects/FTimelineNamedMarker.h"
#include "../Objects/FTimelineTempoMarker.h"
#include "../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes
{
    class TimelineNode
    {
    public:
        Objects::FModGuid BaseGuid;
        Objects::FModGuid LegacyGuid;
        std::vector<Objects::FTriggerBox> TriggerBoxes;
        std::vector<Objects::FTriggerBox> TimeLockedTriggerBoxes;
        std::vector<Objects::FSustainPoint> SustainPoints;
        std::vector<Objects::FTimelineNamedMarker> TimelineNamedMarkers;
        std::vector<Objects::FTimelineTempoMarker> TimelineTempoMarkers;
        std::vector<uint32_t> LegacyUIntArray;

        explicit TimelineNode(Readers::FArchive& Ar) : BaseGuid(Ar)
        {
            if (FModReader::Version() < 0x6d)
                LegacyGuid = Objects::FModGuid(Ar);

            TriggerBoxes = FModReader::ReadElemListImp<Objects::FTriggerBox>(Ar);
            TimeLockedTriggerBoxes = FModReader::ReadElemListImp<Objects::FTriggerBox>(Ar);

            if (FModReader::Version() < 0x84)
                LegacyUIntArray = FModReader::ReadElemListImp(Ar, [](Readers::FArchive& a) { return a.Read<uint32_t>(); });
            else
                SustainPoints = FModReader::ReadVersionedElemListImp<Objects::FSustainPoint>(Ar);

            TimelineNamedMarkers = FModReader::ReadVersionedElemListImp<Objects::FTimelineNamedMarker>(Ar);
            TimelineTempoMarkers = FModReader::ReadElemListImp<Objects::FTimelineTempoMarker>(Ar);
        }
    };
}
