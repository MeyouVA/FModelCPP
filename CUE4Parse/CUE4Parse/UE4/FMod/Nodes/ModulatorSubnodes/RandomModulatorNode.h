// Ported from CUE4Parse/UE4/FMod/Nodes/ModulatorSubnodes/RandomModulatorNode.cs
#pragma once

#include "../../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes::ModulatorSubnodes
{
    class RandomModulatorNode
    {
    public:
        float Amount = 0.0f;
        float Minimum = 0.0f;
        float Maximum = 0.0f;

        explicit RandomModulatorNode(Readers::FArchive& Ar)
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
        }
    };
}
