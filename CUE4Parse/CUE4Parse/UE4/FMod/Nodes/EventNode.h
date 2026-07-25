// Ported from CUE4Parse/UE4/FMod/Nodes/EventNode.cs
#pragma once

#include <vector>

#include "../Objects/FModGuid.h"
#include "../Objects/FParameterId.h"
#include "../Objects/FUserPropertyFloat.h"
#include "../Objects/FUserPropertyString.h"
#include "../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes
{
    class EventNode
    {
    public:
        Objects::FModGuid BaseGuid;
        Objects::FModGuid SnapshotGuid;
        Objects::FModGuid TimelineGuid;
        Objects::FModGuid InputBusGuid;
        Objects::FModGuid MasterTrackGuid;

        int32_t MaximumPolyphony = 0;
        int32_t Priority = 0;
        bool PolyphonyLimitBehavior = false;
        int32_t SchedulingMode = 0;

        std::vector<Objects::FModGuid> ParameterLayouts;

        std::vector<Objects::FUserPropertyFloat> UserPropertyFloatList;
        std::vector<Objects::FUserPropertyString> UserPropertyStringList;

        float DopplerScale = 0.0f;
        float TriggerCooldown = 0.0f;
        uint32_t Flags = 0;

        std::vector<Objects::FModGuid> NonMasterTracks;
        std::vector<Objects::FParameterId> ParameterIds;
        std::vector<Objects::FModGuid> EventTriggeredInstruments;

        float MinimumDistance = 0.0f;
        float MaximumDistance = 0.0f;

        explicit EventNode(Readers::FArchive& Ar)
            : BaseGuid(Ar), SnapshotGuid(Ar), TimelineGuid(Ar), InputBusGuid(Ar), MasterTrackGuid(Ar)
        {
            MaximumPolyphony = Ar.Read<int32_t>();
            Priority = Ar.Read<int32_t>();
            PolyphonyLimitBehavior = Ar.Read<uint8_t>() != 0;
            SchedulingMode = Ar.Read<int32_t>();

            ParameterLayouts = FModReader::ReadElemListImp<Objects::FModGuid>(Ar);

            if (FModReader::Version() < 0x3C) (void) Ar.Read<uint16_t>(); // Dummy elem list

            UserPropertyFloatList = FModReader::ReadVersionedElemListImp<Objects::FUserPropertyFloat>(Ar);
            UserPropertyStringList = FModReader::ReadVersionedElemListImp<Objects::FUserPropertyString>(Ar);

            if (FModReader::Version() >= 0x30) DopplerScale = Ar.Read<float>();
            if (FModReader::Version() >= 0x34) PolyphonyLimitBehavior = Ar.Read<int32_t>() != 0;

            if (FModReader::Version() >= 0x4e) TriggerCooldown = Ar.Read<float>();
            if (FModReader::Version() >= 0x61) Flags = Ar.Read<uint32_t>();

            if (FModReader::Version() >= 0x6b) NonMasterTracks = FModReader::ReadElemListImp<Objects::FModGuid>(Ar);
            if (FModReader::Version() >= 0x76) ParameterIds = FModReader::ReadElemListImp<Objects::FParameterId>(Ar);
            if (FModReader::Version() >= 0x83) EventTriggeredInstruments = FModReader::ReadElemListImp<Objects::FModGuid>(Ar);

            if (FModReader::Version() >= 0x89)
            {
                MinimumDistance = Ar.Read<float>();
                MaximumDistance = Ar.Read<float>();
            }
        }
    };
}
