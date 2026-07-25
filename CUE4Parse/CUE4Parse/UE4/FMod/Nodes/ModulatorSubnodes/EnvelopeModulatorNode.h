// Ported from CUE4Parse/UE4/FMod/Nodes/ModulatorSubnodes/EnvelopeModulatorNode.cs
#pragma once

#include <optional>

#include "../../Objects/FModGuid.h"
#include "../../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes::ModulatorSubnodes
{
    class EnvelopeModulatorNode
    {
    public:
        std::optional<float> Amount;
        float ThresholdMinimum = 0.0f;
        float ThresholdMaximum = 0.0f;
        std::optional<float> AttackTime;
        std::optional<float> ReleaseTime;
        std::optional<bool> UseRMS;
        std::optional<float> Minimum;
        std::optional<float> Maximum;
        std::optional<Objects::FModGuid> EffectId;

        explicit EnvelopeModulatorNode(Readers::FArchive& Ar)
        {
            if (FModReader::Version() >= 0x55)
            {
                Amount = Ar.Read<float>();
            }
            else
            {
                Minimum = Ar.Read<float>();
                Maximum = Ar.Read<float>();
            }

            ThresholdMinimum = Ar.Read<float>();
            ThresholdMaximum = Ar.Read<float>();

            if (FModReader::Version() >= 0x53)
            {
                AttackTime = Ar.Read<float>();
                ReleaseTime = Ar.Read<float>();

                if (FModReader::Version() >= 0x7d)
                    UseRMS = Ar.Read<uint8_t>() != 0;
            }
            else
            {
                EffectId = Objects::FModGuid(Ar);
            }
        }
    };
}
