// Ported from CUE4Parse/UE4/FMod/Nodes/ControllerOwnerNode.cs
#pragma once

#include <vector>

#include "../Objects/FModGuid.h"
#include "../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes
{
    class ControllerOwnerNode
    {
    public:
        std::vector<Objects::FModGuid> Controllers;

        explicit ControllerOwnerNode(Readers::FArchive& Ar)
        {
            Controllers = FModReader::ReadElemListImp<Objects::FModGuid>(Ar);
        }
    };
}
