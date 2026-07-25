// Ported from CUE4Parse/UE4/FMod/Nodes/Effects/EffectNode.cs
#pragma once

#include "../../../Readers/FArchive.h"

namespace CUE4Parse::UE4::FMod::Nodes::Effects
{
    class EffectNode
    {
    public:
        uint32_t Flags = 0;
        float WetMix = 0.0f;
        float WetLevel = 0.0f;
        float DryLevel = 0.0f;
        float InputGain = 0.0f;

        explicit EffectNode(Readers::FArchive& Ar)
        {
            Flags = Ar.Read<uint32_t>();
            WetMix = Ar.Read<float>();
            WetLevel = Ar.Read<float>();
            DryLevel = Ar.Read<float>();
            InputGain = Ar.Read<float>();
        }
    };
}
