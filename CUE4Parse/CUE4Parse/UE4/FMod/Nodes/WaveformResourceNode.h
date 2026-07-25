// Ported from CUE4Parse/UE4/FMod/Nodes/WaveformResourceNode.cs
#pragma once

#include "../Objects/FModGuid.h"
#include "../Enums/EWaveformLoadingMode.h"
#include "../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes
{
    class WaveformResourceNode
    {
    public:
        Objects::FModGuid BaseGuid;
        int32_t SoundBankIndex = 0;
        int32_t SubsoundIndex = 0;
        Enums::EWaveformLoadingMode LoadingMode{};

        explicit WaveformResourceNode(Readers::FArchive& Ar) : BaseGuid(Ar)
        {
            (void) Ar.Read<uint16_t>(); // It's payload size after guid
            SoundBankIndex = Ar.Read<int32_t>();
            SubsoundIndex = Ar.Read<int32_t>();

            if (FModReader::Version() >= 0x46)
                LoadingMode = static_cast<Enums::EWaveformLoadingMode>(Ar.Read<uint32_t>());
        }
    };
}
