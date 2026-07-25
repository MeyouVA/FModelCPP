// Ported from CUE4Parse/UE4/FMod/Nodes/Instruments/ProgrammerInstrumentNode.cs
#pragma once

#include <string>

#include "BaseInstrumentNode.h"
#include "../../Objects/FModGuid.h"
#include "../../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes::Instruments
{
    class ProgrammerInstrumentNode : public BaseInstrumentNode
    {
    public:
        Objects::FModGuid BaseGuid;
        std::string Name;

        explicit ProgrammerInstrumentNode(Readers::FArchive& Ar) : BaseGuid(Ar)
        {
            Name = FModReader::ReadString(Ar);
        }
    };
}
