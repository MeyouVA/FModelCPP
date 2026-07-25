// Ported from CUE4Parse/UE4/FMod/Nodes/Instruments/MultiInstrumentNode.cs
#pragma once

#include <memory>

#include "BaseInstrumentNode.h"
#include "../PlaylistNode.h"
#include "../../Objects/FModGuid.h"

namespace CUE4Parse::UE4::FMod::Nodes::Instruments
{
    class MultiInstrumentNode : public BaseInstrumentNode
    {
    public:
        Objects::FModGuid BaseGuid;
        std::unique_ptr<PlaylistNode> PlaylistBody;

        explicit MultiInstrumentNode(Readers::FArchive& Ar) : BaseGuid(Ar) {}
    };
}
