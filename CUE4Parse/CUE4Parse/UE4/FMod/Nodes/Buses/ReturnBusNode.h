// Ported from CUE4Parse/UE4/FMod/Nodes/Buses/ReturnBusNode.cs
#pragma once

#include "BaseBusNode.h"

namespace CUE4Parse::UE4::FMod::Nodes::Buses
{
    class ReturnBusNode : public BaseBusNode
    {
    public:
        explicit ReturnBusNode(Readers::FArchive& Ar) : BaseBusNode(Ar, true) {}
    };
}
