// Ported from CUE4Parse/UE4/FMod/Nodes/MappingNode.cs
#pragma once

#include <vector>

#include "../Objects/FModGuid.h"
#include "../Objects/FMappingPoint.h"
#include "../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes
{
    class MappingNode
    {
    public:
        Objects::FModGuid BaseGuid;
        std::vector<Objects::FMappingPoint> MappingPoints;

        explicit MappingNode(Readers::FArchive& Ar) : BaseGuid(Ar)
        {
            MappingPoints = FModReader::ReadElemListImp<Objects::FMappingPoint>(Ar);
        }
    };
}
