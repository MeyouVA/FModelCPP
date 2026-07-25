// Ported from CUE4Parse/UE4/FMod/Nodes/ParameterLayoutNode.cs
#pragma once

#include <vector>

#include "../Objects/FModGuid.h"
#include "../Objects/FTriggerBoxParameterLayout.h"
#include "../Objects/FLegacyTriggerBox.h"
#include "../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes
{
    class ParameterLayoutNode
    {
    public:
        Objects::FModGuid BaseGuid;
        Objects::FModGuid ParameterGuid;
        Objects::FModGuid LegacyGuid;
        std::vector<Objects::FModGuid> Instruments;
        uint32_t Flags = 0;
        std::vector<Objects::FTriggerBoxParameterLayout> TriggerBoxes;

        explicit ParameterLayoutNode(Readers::FArchive& Ar) : BaseGuid(Ar), ParameterGuid(Ar)
        {
            if (FModReader::Version() < 0x6d)
                LegacyGuid = Objects::FModGuid(Ar);

            if (FModReader::Version() >= 0x82)
            {
                Instruments = FModReader::ReadElemListImp<Objects::FModGuid>(Ar);
                Flags = Ar.Read<uint32_t>();
                return;
            }

            if (FModReader::Version() >= 0x6a)
            {
                TriggerBoxes = FModReader::ReadElemListImp<Objects::FTriggerBoxParameterLayout>(Ar);
                Flags = Ar.Read<uint32_t>();
                return;
            }
            else
            {
                // Convert legacy trigger boxes to new format
                auto legacy = FModReader::ReadElemListImp<Objects::FLegacyTriggerBox>(Ar);
                TriggerBoxes.reserve(legacy.size());
                for (const auto& l : legacy)
                    TriggerBoxes.emplace_back(l.InstrumentGuid, 0.0f, 0.0f, true);
            }
        }
    };
}
