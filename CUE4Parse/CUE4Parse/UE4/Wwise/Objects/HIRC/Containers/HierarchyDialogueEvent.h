// Ported from CUE4Parse/UE4/Wwise/Objects/HIRC/Containers/HierarchyDialogueEvent.cs
#pragma once

#include <cstdint>
#include <vector>

#include "../../../WwiseArchive.h"
#include "../../../Enums/EAkDecisionTreeMode.h"
#include "../../AkDecisionTree.h"
#include "../../AkGameSync.h"
#include "../../AkPropBundle.h"
#include "../AbstractHierarchy.h"

namespace CUE4Parse::UE4::Wwise::Objects::HIRC::Containers
{
    using CUE4Parse::UE4::Wwise::Enums::EAkDecisionTreeMode;

    // CAkBankMgr::StdBankRead<CAkDialogueEvent>
    class HierarchyDialogueEvent : public AbstractHierarchy
    {
    public:
        uint8_t Probability = 0;
        std::vector<AkGameSync> Arguments;
        EAkDecisionTreeMode Mode = static_cast<EAkDecisionTreeMode>(0);
        AkDecisionTree DecisionTree;
        AkPropBundle PropBundle;

        // CAkDialogueEvent::SetInitialValues
        // Note Probability is read in one of two places depending on version -- before the tree depth
        // past 72, after the tree size for 46..72, and not at all below that.
        explicit HierarchyDialogueEvent(FWwiseArchive& Ar)
        {
            Id = Ar.Read<uint32_t>();
            if (Ar.Version > 72)
                Probability = Ar.Read<uint8_t>();

            const uint32_t treeDepth = Ar.Read<uint32_t>();
            Arguments = AkGameSync::ReadSequential(Ar, treeDepth);

            const uint32_t treeSize = Ar.Read<uint32_t>();

            if (Ar.Version > 45 && Ar.Version <= 72)
                Probability = Ar.Read<uint8_t>();

            if (Ar.Version > 45)
                Mode = Ar.Read<EAkDecisionTreeMode>();

            DecisionTree = AkDecisionTree(Ar, treeDepth, treeSize);
            PropBundle = AkPropBundle(Ar);
        }
    };
}
