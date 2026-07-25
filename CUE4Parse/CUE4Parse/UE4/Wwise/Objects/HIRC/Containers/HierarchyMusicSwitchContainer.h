// Ported from CUE4Parse/UE4/Wwise/Objects/HIRC/Containers/HierarchyMusicSwitchContainer.cs
#pragma once

#include <cstdint>
#include <vector>

#include "../../../WwiseArchive.h"
#include "../../../Enums/EAkDecisionTreeMode.h"
#include "../../../Enums/EAkGroupType.h"
#include "../../AkDecisionTree.h"
#include "../../AkGameSync.h"
#include "../../AkMeterInfo.h"
#include "../../AkMusicSwitchAssoc.h"
#include "../../AkMusicTransitionRule.h"
#include "../../AkStinger.h"
#include "../BaseHierarchyMusic.h"

namespace CUE4Parse::UE4::Wwise::Objects::HIRC::Containers
{
    using CUE4Parse::UE4::Wwise::Enums::EAkDecisionTreeMode;
    using CUE4Parse::UE4::Wwise::Enums::EAkGroupType;

    class HierarchyMusicSwitchContainer : public BaseHierarchyMusic
    {
    public:
        AkMeterInfo MeterInfo;
        std::vector<AkStinger> Stingers;
        AkMusicTransitionRule MusicTransitionRule;

        // The <= 72 shape: a flat switch/association list.
        EAkGroupType GroupType = static_cast<EAkGroupType>(0);
        uint32_t GroupId = 0;
        uint32_t DefaultSwitch = 0;
        bool IsContinuousValidation = false;
        std::vector<AkMusicSwitchAssoc> MusicSwitchAssoc;

        // The > 72 shape: a decision tree keyed by game syncs. The two are mutually exclusive.
        uint8_t IsContinuePlayback = 0;
        std::vector<AkGameSync> Arguments;
        EAkDecisionTreeMode Mode = static_cast<EAkDecisionTreeMode>(0);
        AkDecisionTree DecisionTree;

        // CAkMusicSwitchCntr::SetInitialValues
        explicit HierarchyMusicSwitchContainer(FWwiseArchive& Ar) : BaseHierarchyMusic(Ar)
        {
            MeterInfo = AkMeterInfo(Ar);
            Stingers = AkStinger::ReadArray(Ar);

            MusicTransitionRule = AkMusicTransitionRule(Ar);

            if (Ar.Version <= 72)
            {
                DecisionTree = AkDecisionTree(); // Empty tree for old versions
                GroupType = static_cast<EAkGroupType>(Ar.Read<uint32_t>());
                GroupId = Ar.Read<uint32_t>();
                DefaultSwitch = Ar.Read<uint32_t>();
                IsContinuousValidation = Ar.ReadBool();
                MusicSwitchAssoc = Ar.ReadArrayWith([&Ar] { return AkMusicSwitchAssoc(Ar); });
            }
            else
            {
                IsContinuePlayback = Ar.Read<uint8_t>();

                const uint32_t treeDepth = Ar.Read<uint32_t>();
                Arguments = AkGameSync::ReadSequential(Ar, treeDepth);

                const uint32_t treeDataSize = Ar.Read<uint32_t>();
                Mode = Ar.Read<EAkDecisionTreeMode>();

                DecisionTree = AkDecisionTree(Ar, treeDepth, treeDataSize);
            }
        }
    };
}
