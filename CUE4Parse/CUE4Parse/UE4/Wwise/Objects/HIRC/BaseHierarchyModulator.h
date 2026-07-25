// Ported from CUE4Parse/UE4/Wwise/Objects/HIRC/BaseHierarchyModulator.cs
#pragma once

#include <vector>

#include "../../WwiseArchive.h"
#include "../AkPropBundle.h"
#include "../AkRTPC.h"
#include "AbstractHierarchy.h"

namespace CUE4Parse::UE4::Wwise::Objects::HIRC
{
    // CAkModulator
    class BaseHierarchyModulator : public AbstractHierarchy
    {
    public:
        std::vector<AkProp> Props;
        std::vector<AkPropRange> PropRanges;
        std::vector<AkRtpc> RtpcCurves;

        // CAkModulator::SetInitialValues
        explicit BaseHierarchyModulator(FWwiseArchive& Ar)
        {
            Id = Ar.Read<uint32_t>();
            Props = AkPropBundle::ReadSequentialAkProp(Ar);
            PropRanges = AkPropBundle::ReadSequentialAkPropRange(Ar);
            RtpcCurves = AkRtpc::ReadArray(Ar);
        }
    };
}
