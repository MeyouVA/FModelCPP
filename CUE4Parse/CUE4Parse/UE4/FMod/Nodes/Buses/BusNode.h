// Ported from CUE4Parse/UE4/FMod/Nodes/Buses/BusNode.cs
#pragma once

#include <vector>

#include "../../Objects/FModGuid.h"
#include "../../Objects/FMixerStrip.h"
#include "../../Enums/EPortType.h"
#include "../../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes::Buses
{
    class BusNode
    {
    public:
        uint32_t Flags = 0;
        uint32_t InputChannelLayout = 0;
        std::vector<Objects::FModGuid> PreFaderEffects;
        std::vector<Objects::FModGuid> PostFaderEffects;
        Objects::FMixerStrip MixerStrip;

        int32_t MaximumPolyphony = 0;
        int32_t PolyphonyLimitBehavior = 0;
        std::vector<uint32_t> PreFaderInputChannelLayouts;
        std::vector<uint32_t> PostFaderInputChannelLayouts;

        int32_t ObjectPannerIndex = 0;
        Enums::EPortType PortType{};

        explicit BusNode(Readers::FArchive& Ar)
        {
            if (FModReader::Version() >= 0x8c)
                Flags = Ar.Read<uint32_t>();
            else
                Flags = Ar.Read<uint8_t>();

            InputChannelLayout = Ar.Read<uint32_t>();
            PreFaderEffects = FModReader::ReadElemListImp<Objects::FModGuid>(Ar);
            PostFaderEffects = FModReader::ReadElemListImp<Objects::FModGuid>(Ar);
            MixerStrip = Objects::FMixerStrip(Ar);

            if (FModReader::Version() >= 0x42)
            {
                MaximumPolyphony = Ar.Read<int32_t>();
                PolyphonyLimitBehavior = Ar.Read<int32_t>();
            }

            if (FModReader::Version() >= 0x5B)
            {
                PreFaderInputChannelLayouts = FModReader::ReadElemListImp(Ar, [](Readers::FArchive& a) { return a.Read<uint32_t>(); });
                PostFaderInputChannelLayouts = FModReader::ReadElemListImp(Ar, [](Readers::FArchive& a) { return a.Read<uint32_t>(); });
            }

            if (FModReader::Version() >= 0x6f)
                ObjectPannerIndex = Ar.Read<int32_t>();

            if (FModReader::Version() >= 0x8c)
                PortType = static_cast<Enums::EPortType>(Ar.Read<uint32_t>());
        }
    };
}
