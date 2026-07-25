// Ported from CUE4Parse/UE4/FMod/Nodes/ModulatorSubnodes/SeekModulatorNode.cs
#pragma once

#include "../../../Readers/FArchive.h"

namespace CUE4Parse::UE4::FMod::Nodes::ModulatorSubnodes
{
    class SeekModulatorNode
    {
    public:
        uint32_t Flags = 0;
        float SeekSpeedAscending = 0.0f;
        float SeekSpeedDescending = 0.0f;

        explicit SeekModulatorNode(Readers::FArchive& Ar)
        {
            Flags = Ar.Read<uint32_t>();
            SeekSpeedAscending = Ar.Read<float>();
            SeekSpeedDescending = Ar.Read<float>();
        }
    };
}
