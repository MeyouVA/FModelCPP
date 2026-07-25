// Ported from CUE4Parse/UE4/FMod/Nodes/Buses/BaseBusNode.cs
// Base for the bus node family. C# holds a nullable BusNode reference set after construction; the port
// owns it via unique_ptr. Polymorphic (stored in FModReader::BusNodes as unique_ptr<BaseBusNode>).
#pragma once

#include <memory>

#include "../../Objects/FModGuid.h"
#include "../../Objects/FRoutable.h"
#include "BusNode.h"

namespace CUE4Parse::UE4::FMod::Nodes::Buses
{
    class BaseBusNode
    {
    public:
        Objects::FModGuid BaseGuid;
        Objects::FRoutable Routable;
        std::unique_ptr<BusNode> BusBody;

        BaseBusNode(Readers::FArchive& Ar, bool includeRoutable) : BaseGuid(Ar)
        {
            if (includeRoutable) Routable = Objects::FRoutable(Ar);
        }

        virtual ~BaseBusNode() = default;
    };
}
