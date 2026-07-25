// Ported from CUE4Parse/UE4/FMod/Nodes/Buses/GroupBusNode.cs
#pragma once

#include "BaseBusNode.h"

namespace CUE4Parse::UE4::FMod::Nodes::Buses
{
    class GroupBusNode : public BaseBusNode
    {
    public:
        explicit GroupBusNode(Readers::FArchive& Ar) : BaseBusNode(Ar, true) {}
    };
}
