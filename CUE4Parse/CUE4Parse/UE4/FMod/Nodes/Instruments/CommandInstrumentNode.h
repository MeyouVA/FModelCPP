// Ported from CUE4Parse/UE4/FMod/Nodes/Instruments/CommandInstrumentNode.cs
#pragma once

#include "BaseInstrumentNode.h"
#include "../../Objects/FModGuid.h"
#include "../../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes::Instruments
{
    class CommandInstrumentNode : public BaseInstrumentNode
    {
    public:
        Objects::FModGuid BaseGuid;
        uint32_t CommandType = 0;
        Objects::FModGuid TargetGuid;
        float Value = 0.0f;

        explicit CommandInstrumentNode(Readers::FArchive& Ar) : BaseGuid(Ar)
        {
            CommandType = Ar.Read<uint32_t>();
            TargetGuid = Objects::FModGuid(Ar);

            if (FModReader::Version() >= 0x80)
                Value = Ar.Read<float>();
        }
    };
}
