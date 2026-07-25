// Ported from CUE4Parse/UE4/FMod/Nodes/Instruments/WaveformInstrumentNode.cs
#pragma once

#include "BaseInstrumentNode.h"
#include "../../Objects/FModGuid.h"
#include "../../Enums/EWaveformLoadingMode.h"
#include "../../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes::Instruments
{
    class WaveformInstrumentNode : public BaseInstrumentNode
    {
    public:
        Objects::FModGuid BaseGuid;
        Enums::EWaveformLoadingMode LegacyLoadingMode{};
        Objects::FModGuid WaveformResourceGuid;

        explicit WaveformInstrumentNode(Readers::FArchive& Ar) : BaseGuid(Ar)
        {
            if (FModReader::Version() < 0x46)
                LegacyLoadingMode = static_cast<Enums::EWaveformLoadingMode>(Ar.Read<uint32_t>());
            WaveformResourceGuid = Objects::FModGuid(Ar);
        }
    };
}
