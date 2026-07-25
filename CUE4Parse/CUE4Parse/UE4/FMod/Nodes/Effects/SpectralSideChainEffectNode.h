// Ported from CUE4Parse/UE4/FMod/Nodes/Effects/SpectralSideChainEffectNode.cs
#pragma once

#include <vector>

#include "BaseEffectNode.h"
#include "../../Objects/FModGuid.h"
#include "../../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes::Effects
{
    class SpectralSideChainEffectNode : public BaseEffectNode
    {
    public:
        Objects::FModGuid BaseGuid;
        float Level = 0.0f;
        float MinimumFrequency = 0.0f;
        float MaximumFrequency = 0.0f;
        uint32_t Flags = 0;
        std::vector<Objects::FModGuid> Targets;

        explicit SpectralSideChainEffectNode(Readers::FArchive& Ar) : BaseGuid(Ar)
        {
            Level = Ar.Read<float>();
            MinimumFrequency = Ar.Read<float>();
            MaximumFrequency = Ar.Read<float>();
            Flags = Ar.Read<uint32_t>();
            (void) Ar.ReadBytes(8);
            Targets = FModReader::ReadElemListImp<Objects::FModGuid>(Ar);
        }
    };
}
