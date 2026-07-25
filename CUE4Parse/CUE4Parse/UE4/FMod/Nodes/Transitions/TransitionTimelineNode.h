// Ported from CUE4Parse/UE4/FMod/Nodes/Transitions/TransitionTimelineNode.cs
#pragma once

#include <vector>

#include "../../Objects/FModGuid.h"
#include "../../Objects/FTriggerBox.h"
#include "../../Objects/FFadeCurve.h"
#include "../../Objects/FControllerOverride.h"
#include "../../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes::Transitions
{
    class TransitionTimelineNode
    {
    public:
        uint32_t Length = 0;
        std::vector<Objects::FTriggerBox> TimeLockedTriggerBoxes;
        std::vector<Objects::FTriggerBox> TriggeredTriggerBoxes;
        uint32_t LeadInLength = 0;
        uint32_t LeadOutLength = 0;
        std::vector<Objects::FFadeCurve> LeadInCurves;
        std::vector<Objects::FFadeCurve> LeadOutCurves;
        Objects::FModGuid CurveMappingGuid;
        std::vector<Objects::FControllerOverride> FadeOverrides;

        explicit TransitionTimelineNode(Readers::FArchive& Ar)
        {
            Length = Ar.Read<uint32_t>();
            FadeOverrides = FModReader::ReadElemListImp<Objects::FControllerOverride>(Ar);
            TimeLockedTriggerBoxes = FModReader::ReadElemListImp<Objects::FTriggerBox>(Ar);
            TriggeredTriggerBoxes = FModReader::ReadElemListImp<Objects::FTriggerBox>(Ar);

            LeadInLength = Length;

            if (FModReader::Version() < 0x3E) return;

            LeadInLength = Ar.Read<uint32_t>();
            LeadOutLength = Ar.Read<uint32_t>();
            LeadInCurves = FModReader::ReadElemListImp<Objects::FFadeCurve>(Ar);
            LeadOutCurves = FModReader::ReadElemListImp<Objects::FFadeCurve>(Ar);
            CurveMappingGuid = Objects::FModGuid(Ar);

            if (FModReader::Version() >= 0x7E)
                FadeOverrides = FModReader::ReadElemListImp<Objects::FControllerOverride>(Ar);
        }
    };
}
