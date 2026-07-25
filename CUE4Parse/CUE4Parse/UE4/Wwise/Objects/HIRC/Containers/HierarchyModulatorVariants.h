// Ported from HierarchyLFO.cs, HierarchyEnvelope.cs and HierarchyTimeMod.cs -- the three
// BaseHierarchyModulator subclasses, none of which adds a field.
#pragma once

#include "../../../WwiseArchive.h"
#include "../BaseHierarchyModulator.h"

namespace CUE4Parse::UE4::Wwise::Objects::HIRC::Containers
{
    // CAkBankMgr::StdBankRead<CAkLFOModulator>
    class HierarchyLFO : public BaseHierarchyModulator
    {
    public:
        explicit HierarchyLFO(FWwiseArchive& Ar) : BaseHierarchyModulator(Ar) {}
    };

    // CAkBankMgr::StdBankRead<CAkEnvelopeModulator>
    class HierarchyEnvelope : public BaseHierarchyModulator
    {
    public:
        explicit HierarchyEnvelope(FWwiseArchive& Ar) : BaseHierarchyModulator(Ar) {}
    };

    // CAkBankMgr::StdBankRead<CAkTimeModulator>
    class HierarchyTimeMod : public BaseHierarchyModulator
    {
    public:
        explicit HierarchyTimeMod(FWwiseArchive& Ar) : BaseHierarchyModulator(Ar) {}
    };
}
