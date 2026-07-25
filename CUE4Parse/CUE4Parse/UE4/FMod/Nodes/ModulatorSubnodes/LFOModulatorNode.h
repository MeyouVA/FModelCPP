// Ported from CUE4Parse/UE4/FMod/Nodes/ModulatorSubnodes/LFOModulatorNode.cs
#pragma once

#include "../../../Readers/FArchive.h"

namespace CUE4Parse::UE4::FMod::Nodes::ModulatorSubnodes
{
    class LFOModulatorNode
    {
    public:
        uint32_t Shape = 0;
        uint32_t Flags = 0;
        float Rate = 0.0f;
        float Amount = 0.0f;
        float Phase = 0.0f;
        float Direction = 0.0f;

        explicit LFOModulatorNode(Readers::FArchive& Ar)
        {
            Shape = Ar.Read<uint32_t>();      // 0x50
            Flags = Ar.Read<uint32_t>();      // 0x54
            Rate = Ar.Read<float>();          // 0x58
            Amount = Ar.Read<float>();        // 0x5C
            Phase = Ar.Read<float>();         // 0x60
            Direction = Ar.Read<float>();     // 0x64
        }
    };
}
