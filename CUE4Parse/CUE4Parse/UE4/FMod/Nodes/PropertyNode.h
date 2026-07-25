// Ported from CUE4Parse/UE4/FMod/Nodes/PropertyNode.cs
#pragma once

#include <vector>

#include "../Objects/FModGuid.h"
#include "../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes
{
    class PropertyNode
    {
    public:
        uint32_t Index = 0;
        uint16_t Method = 0;
        uint16_t Type = 0;
        Objects::FModGuid MappingGuid;
        std::vector<Objects::FModGuid> Controllers;
        std::vector<Objects::FModGuid> Modulators;

        explicit PropertyNode(Readers::FArchive& Ar)
        {
            Index = Ar.Read<uint32_t>();
            Method = Ar.Read<uint16_t>();
            Type = Ar.Read<uint16_t>();
            MappingGuid = Objects::FModGuid(Ar);
            Controllers = FModReader::ReadElemListImp<Objects::FModGuid>(Ar);
            Modulators = FModReader::ReadElemListImp<Objects::FModGuid>(Ar);
        }
    };
}
