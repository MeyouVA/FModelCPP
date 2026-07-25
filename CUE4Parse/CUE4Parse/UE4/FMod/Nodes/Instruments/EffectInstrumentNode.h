// Ported from CUE4Parse/UE4/FMod/Nodes/Instruments/EffectInstrumentNode.cs
#pragma once

#include "BaseInstrumentNode.h"
#include "../../Objects/FModGuid.h"

namespace CUE4Parse::UE4::FMod::Nodes::Instruments
{
    class EffectInstrumentNode : public BaseInstrumentNode
    {
    public:
        Objects::FModGuid BaseGuid;
        Objects::FModGuid EffectGuid;

        explicit EffectInstrumentNode(Readers::FArchive& Ar) : BaseGuid(Ar), EffectGuid(Ar) {}
    };
}
