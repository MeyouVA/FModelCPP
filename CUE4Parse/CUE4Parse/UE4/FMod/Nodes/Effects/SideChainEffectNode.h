// Ported from CUE4Parse/UE4/FMod/Nodes/Effects/SideChainEffectNode.cs
#pragma once

#include <vector>

#include "BaseEffectNode.h"
#include "../../Objects/FModGuid.h"
#include "../../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes::Effects
{
    class SideChainEffectNode : public BaseEffectNode
    {
    public:
        Objects::FModGuid BaseGuid;
        bool IsActive = false;
        std::vector<Objects::FModGuid> Targets;
        uint32_t InputChannelLayout = 0;
        std::vector<Objects::FModGuid> Modulators;
        float SideChainLevel = 0.0f;

        explicit SideChainEffectNode(Readers::FArchive& Ar) : BaseGuid(Ar)
        {
            IsActive = Ar.Read<uint8_t>() != 0;
            Targets = FModReader::ReadElemListImp<Objects::FModGuid>(Ar);
            if (FModReader::Version() >= 0x4A && FModReader::Version() <= 0x5A)
                InputChannelLayout = Ar.Read<uint32_t>();
            if (FModReader::Version() >= 0x53)
                Modulators = FModReader::ReadElemListImp<Objects::FModGuid>(Ar);
            if (FModReader::Version() >= 0x88)
                SideChainLevel = Ar.Read<float>();
        }
    };
}
