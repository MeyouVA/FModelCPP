// Ported from CUE4Parse/UE4/FMod/Nodes/Buses/MasterBusNode.cs
#pragma once

#include "BaseBusNode.h"
#include "../../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes::Buses
{
    class MasterBusNode : public BaseBusNode
    {
    public:
        explicit MasterBusNode(Readers::FArchive& Ar) : BaseBusNode(Ar, FModReader::Version() >= 0x49) {}
    };
}
