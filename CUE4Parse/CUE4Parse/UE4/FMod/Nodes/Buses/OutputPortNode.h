// Ported from CUE4Parse/UE4/FMod/Nodes/Buses/OutputPortNode.cs
#pragma once

#include "BaseBusNode.h"

namespace CUE4Parse::UE4::FMod::Nodes::Buses
{
    class OutputPortNode : public BaseBusNode
    {
    public:
        explicit OutputPortNode(Readers::FArchive& Ar) : BaseBusNode(Ar, true) {}
    };
}
