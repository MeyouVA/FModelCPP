// Ported from CUE4Parse/UE4/FMod/Nodes/Instruments/SilenceInstrumentNode.cs
#pragma once

#include "BaseInstrumentNode.h"
#include "../../Objects/FModGuid.h"

namespace CUE4Parse::UE4::FMod::Nodes::Instruments
{
    class SilenceInstrumentNode : public BaseInstrumentNode
    {
    public:
        Objects::FModGuid BaseGuid;
        float Duration = 0.0f;

        explicit SilenceInstrumentNode(Readers::FArchive& Ar) : BaseGuid(Ar)
        {
            Duration = Ar.Read<float>();
        }
    };
}
