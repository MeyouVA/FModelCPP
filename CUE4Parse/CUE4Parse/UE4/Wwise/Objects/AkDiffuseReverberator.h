// Ported from CUE4Parse/UE4/Wwise/Objects/AkDiffuseReverberator.cs
#pragma once

#include <cstdint>

#include "../WwiseArchive.h"
#include "ICAkIndexable.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    // > 118 <= 122, later removed
    struct AkDiffuseReverberator : ICAkIndexable
    {
        uint32_t Id = 0;
        float Time = 0;
        float HFRatio = 0;
        float DryLevel = 0;
        float WetLevel = 0;
        float Spread = 0;
        float Density = 0;
        float Quality = 0;
        float Diffusion = 0;
        float Scale = 0;

        AkDiffuseReverberator() = default;

        explicit AkDiffuseReverberator(FWwiseArchive& Ar)
        {
            Id = Ar.Read<uint32_t>();
            Time = Ar.Read<float>();
            HFRatio = Ar.Read<float>();
            DryLevel = Ar.Read<float>();
            WetLevel = Ar.Read<float>();
            Spread = Ar.Read<float>();
            Density = Ar.Read<float>();
            Quality = Ar.Read<float>();
            Diffusion = Ar.Read<float>();
            Scale = Ar.Read<float>();
        }
    };
}
