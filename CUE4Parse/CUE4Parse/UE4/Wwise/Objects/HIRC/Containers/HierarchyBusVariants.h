// Ported from four one-line C# files that all derive from BaseHierarchyBus and add nothing:
//   HierarchyAudioBus.cs, HierarchyAuxiliaryBus.cs, HierarchyFeedbackBus.cs, HierarchyFeedbackNode.cs
// They exist so the HIRC type dispatch stays one class per type. Kept as four distinct types in one
// header rather than four near-empty ones.
#pragma once

#include "../../../WwiseArchive.h"
#include "../BaseHierarchyBus.h"

namespace CUE4Parse::UE4::Wwise::Objects::HIRC::Containers
{
    // CAkBankMgr::ReadBus
    class HierarchyAudioBus : public BaseHierarchyBus
    {
    public:
        explicit HierarchyAudioBus(FWwiseArchive& Ar) : BaseHierarchyBus(Ar) {}
    };

    // CAkBankMgr::StdBankRead<CAkAuxBus>
    class HierarchyAuxiliaryBus : public BaseHierarchyBus
    {
    public:
        explicit HierarchyAuxiliaryBus(FWwiseArchive& Ar) : BaseHierarchyBus(Ar) {}
    };

    // Legacy HIRC <= 125
    class HierarchyFeedbackBus : public BaseHierarchyBus
    {
    public:
        explicit HierarchyFeedbackBus(FWwiseArchive& Ar) : BaseHierarchyBus(Ar) {}
    };

    // Legacy HIRC <= 125
    // C# carries a "TODO: Won't be read correctly" here -- the feedback node is not really a bus, so the
    // inherited reader consumes the wrong fields. Kept as-is; Hierarchy's length fixup absorbs it.
    class HierarchyFeedbackNode : public BaseHierarchyBus
    {
    public:
        explicit HierarchyFeedbackNode(FWwiseArchive& Ar) : BaseHierarchyBus(Ar) {}
    };
}
