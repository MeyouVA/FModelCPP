// Ported from CUE4Parse/UE4/FMod/Nodes/Instruments/BaseInstrumentNode.cs
// Abstract base for the instrument node family; InstrumentBody is attached after construction. Stored in
// FModReader::InstrumentNodes as unique_ptr<BaseInstrumentNode>, so it carries a virtual destructor.
#pragma once

#include <memory>

#include "InstrumentNode.h"

namespace CUE4Parse::UE4::FMod::Nodes::Instruments
{
    class BaseInstrumentNode
    {
    public:
        std::unique_ptr<InstrumentNode> InstrumentBody;

        virtual ~BaseInstrumentNode() = default;
    };
}
