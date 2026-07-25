// Ported from CUE4Parse/UE4/FMod/Nodes/Instruments/EventInstrumentNode.cs
#pragma once

#include <vector>

#include "BaseInstrumentNode.h"
#include "../../Objects/FModGuid.h"
#include "../../Objects/FEventParameterStub.h"
#include "../../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes::Instruments
{
    class EventInstrumentNode : public BaseInstrumentNode
    {
    public:
        Objects::FModGuid BaseGuid;
        Objects::FModGuid EventGuid;
        float SnapshotIntensity = 0.0f;
        std::vector<Objects::FEventParameterStub> EventParameterStubs;

        explicit EventInstrumentNode(Readers::FArchive& Ar) : BaseGuid(Ar), EventGuid(Ar)
        {
            SnapshotIntensity = Ar.Read<float>();
            EventParameterStubs = FModReader::ReadElemListImp<Objects::FEventParameterStub>(Ar);
        }
    };
}
