// Ported from CUE4Parse/UE4/FMod/Nodes/Buses/InputBusNode.cs
#pragma once

#include "BaseBusNode.h"

namespace CUE4Parse::UE4::FMod::Nodes::Buses
{
    class InputBusNode : public BaseBusNode
    {
    public:
        explicit InputBusNode(Readers::FArchive& Ar) : BaseBusNode(Ar, true) {}
    };
}
