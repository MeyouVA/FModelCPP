// Ported from CUE4Parse/UE4/Wwise/Objects/HIRC/BaseHierarchy.cs
#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "../../WwiseArchive.h"
#include "../../Enums/EAkBelowThresholdBehavior.h"
#include "../../Enums/EAkVirtualQueueBehavior.h"
#include "../../Enums/Flags/EHdrEnvelopeFlags.h"
#include "../../Enums/Flags/EMidiBehaviorFlags.h"
#include "../AkAdvSettingsParams.h"
#include "../AkAuxParams.h"
#include "../AkFXParams.h"
#include "../AkFeedbackInfo.h"
#include "../AkPositioningParams.h"
#include "../AkPropBundle.h"
#include "../AkRTPC.h"
#include "../AkStateChunk.h"
#include "AbstractHierarchy.h"

namespace CUE4Parse::UE4::Wwise::Objects::HIRC
{
    using CUE4Parse::UE4::Wwise::Enums::EAkBelowThresholdBehavior;
    using CUE4Parse::UE4::Wwise::Enums::EAkVirtualQueueBehavior;
    using CUE4Parse::UE4::Wwise::Enums::Flags::EHdrEnvelopeFlags;
    using CUE4Parse::UE4::Wwise::Enums::Flags::EMidiBehaviorFlags;

    // CAkParameterNodeBase
    class BaseHierarchy : public AbstractHierarchy
    {
    public:
        bool OverrideFx = false;
        AkFxParams FxParams;
        bool OverrideParentMetadataFlag = false;
        std::vector<AkFxChunk> FxChunks;
        bool OverrideAttachmentParams = false;
        uint32_t OverrideBusId = 0;
        uint32_t DirectParentId = 0;
        bool Priority = false;
        bool PriorityOverrideParent = false;
        bool PriorityApplyDistFactor = false;
        int8_t DistOffset = 0;
        EMidiBehaviorFlags MidiBehaviorFlags = static_cast<EMidiBehaviorFlags>(0);
        AkPropBundle PropBundle;
        AkPositioningParams PositioningParams;
        std::optional<AkAuxParams> AuxParams;
        AkAdvSettingsParams AdvSettingsParams;
        // Faithful quirk: C# declares these four and never assigns them -- the real values live inside
        // AdvSettingsParams. They stay here (default-valued) so the shape matches.
        EAkVirtualQueueBehavior VirtualQueueBehavior = static_cast<EAkVirtualQueueBehavior>(0);
        uint16_t MaxNumInstance = 0;
        EAkBelowThresholdBehavior BelowThresholdBehavior = static_cast<EAkBelowThresholdBehavior>(0);
        EHdrEnvelopeFlags HdrEnvelopeFlags = static_cast<EHdrEnvelopeFlags>(0);
        std::vector<AkStateGroup> StateGroups;
        std::vector<AkRtpc> RtpcList;
        std::optional<AkFeedbackInfo> FeedbackInfo;

        BaseHierarchy() = default;

        // CAkParameterNodeBase::SetNodeBaseParams
        explicit BaseHierarchy(FWwiseArchive& Ar)
        {
            OverrideFx = Ar.ReadBool();
            FxParams = AkFxParams(Ar);

            if (Ar.Version > 136)
            {
                OverrideParentMetadataFlag = Ar.ReadBool();
                const int count = Ar.Read<uint8_t>();
                FxChunks = Ar.ReadArrayWith(count, [&Ar] { return AkFxChunk(Ar); });
            }

            if (Ar.Version > 89 && Ar.Version <= 145)
            {
                OverrideAttachmentParams = Ar.ReadBool();
            }

            OverrideBusId = Ar.Read<uint32_t>();
            DirectParentId = Ar.Read<uint32_t>();

            if (Ar.Version <= 56)
            {
                Priority = Ar.ReadBool();
                PriorityOverrideParent = Ar.ReadBool();
                PriorityApplyDistFactor = Ar.ReadBool();
                DistOffset = Ar.Read<int8_t>();
            }
            else if (Ar.Version <= 89)
            {
                PriorityOverrideParent = Ar.ReadBool();
                PriorityApplyDistFactor = Ar.ReadBool();
            }
            else
            {
                MidiBehaviorFlags = Ar.Read<EMidiBehaviorFlags>();
                // Faithful quirk: C# compares the whole flag word for *equality* rather than testing the
                // bit, so both are only true when no other flag is set. Kept as-is.
                PriorityOverrideParent = MidiBehaviorFlags == EMidiBehaviorFlags::PriorityOverrideParent;
                PriorityApplyDistFactor = MidiBehaviorFlags == EMidiBehaviorFlags::PriorityApplyDistFactor;
            }

            PropBundle = AkPropBundle(Ar);

            PositioningParams = AkPositioningParams(Ar);

            if (Ar.Version > 65)
            {
                AuxParams = AkAuxParams(Ar);
            }

            AdvSettingsParams = AkAdvSettingsParams(Ar);

            // C# splits <= 52 and <= 122 into separate arms with a "state chunk inlined" TODO, but both
            // read the same thing today.
            if (Ar.Version <= 122)
            {
                StateGroups = AkStateChunk(Ar).Groups;
            }
            else
            {
                StateGroups = AkStateAwareChunk(Ar).Groups;
            }

            RtpcList = AkRtpc::ReadArray(Ar);

            // Only present when the bank header said feedback data is embedded -- hence HasFeedback on
            // the archive rather than a local.
            if (Ar.Version <= 126 && Ar.HasFeedback)
            {
                FeedbackInfo = AkFeedbackInfo(Ar);
            }
        }
    };
}
