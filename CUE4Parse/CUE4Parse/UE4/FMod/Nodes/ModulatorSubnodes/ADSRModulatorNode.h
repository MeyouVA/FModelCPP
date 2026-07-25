// Ported from CUE4Parse/UE4/FMod/Nodes/ModulatorSubnodes/ADSRModulatorNode.cs
#pragma once

#include <optional>

#include "../../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes::ModulatorSubnodes
{
    /// ADSR (Attack, Decay, Sustain, Release) modulator
    class ADSRModulatorNode
    {
    public:
        float InitialValue = 0.0f;
        float PeakValue = 0.0f;
        float SustainValue = 0.0f;
        float AttackTime = 0.0f;
        float HoldTime = 0.0f;
        float DecayTime = 0.0f;
        float ReleaseTime = 0.0f;
        float AttackShape = 0.0f;
        float DecayShape = 0.0f;
        float ReleaseShape = 0.0f;
        std::optional<float> FinalValue;

        explicit ADSRModulatorNode(Readers::FArchive& Ar)
        {
            InitialValue = Ar.Read<float>();
            PeakValue = Ar.Read<float>();
            SustainValue = Ar.Read<float>();
            AttackTime = Ar.Read<float>();
            HoldTime = Ar.Read<float>();
            DecayTime = Ar.Read<float>();
            ReleaseTime = Ar.Read<float>();
            AttackShape = Ar.Read<float>();
            DecayShape = Ar.Read<float>();
            ReleaseShape = Ar.Read<float>();

            if (FModReader::Version() >= 0x74)
                FinalValue = Ar.Read<float>();
        }
    };
}
