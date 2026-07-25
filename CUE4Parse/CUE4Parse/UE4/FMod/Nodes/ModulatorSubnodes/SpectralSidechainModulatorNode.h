// Ported from CUE4Parse/UE4/FMod/Nodes/ModulatorSubnodes/SpectralSidechainModulatorNode.cs
#pragma once

#include "../../Objects/FModGuid.h"
#include "../../Enums/ESpectralSidechainModulatorMode.h"

namespace CUE4Parse::UE4::FMod::Nodes::ModulatorSubnodes
{
    class SpectralSidechainModulatorNode
    {
    public:
        float Amount = 0.0f;
        Enums::ESpectralSidechainModulatorMode Mode{};
        float ThresholdMinimum = 0.0f;
        float ThresholdMaximum = 0.0f;
        float AttackTime = 0.0f;
        float ReleaseTime = 0.0f;
        Objects::FModGuid ThresholdMapping;

        explicit SpectralSidechainModulatorNode(Readers::FArchive& Ar)
        {
            Amount = Ar.Read<float>();
            Mode = static_cast<Enums::ESpectralSidechainModulatorMode>(Ar.Read<int32_t>());
            ThresholdMinimum = Ar.Read<float>();
            ThresholdMaximum = Ar.Read<float>();
            AttackTime = Ar.Read<float>();
            ReleaseTime = Ar.Read<float>();

            ThresholdMapping = Objects::FModGuid(Ar);
        }
    };
}
