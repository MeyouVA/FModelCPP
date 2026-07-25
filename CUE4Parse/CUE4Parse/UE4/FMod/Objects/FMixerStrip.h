// Ported from CUE4Parse/UE4/FMod/Objects/FMixerStrip.cs
#pragma once

#include <vector>

#include "FModGuid.h"
#include "../FModReader.h"

namespace CUE4Parse::UE4::FMod::Objects
{
    struct FMixerStrip
    {
        float Volume = 0.0f;
        float Pitch = 0.0f;
        std::vector<FModGuid> VCAs;

        FMixerStrip() = default;
        explicit FMixerStrip(Readers::FArchive& Ar)
        {
            (void) Ar.Read<uint16_t>(); // Payload size
            Volume = Ar.Read<float>();
            Pitch = Ar.Read<float>();

            if (FModReader::Version() >= 0x6c)
                VCAs = FModReader::ReadElemListImp<FModGuid>(Ar);
        }
    };
}
