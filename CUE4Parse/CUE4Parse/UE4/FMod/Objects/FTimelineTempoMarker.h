// Ported from CUE4Parse/UE4/FMod/Objects/FTimelineTempoMarker.cs
#pragma once

#include "FModGuid.h"

namespace CUE4Parse::UE4::FMod::Objects
{
    struct FTimelineTempoMarker
    {
        FModGuid BaseGuid;
        int64_t TimeSignature = 0;
        uint32_t Position = 0;
        float Tempo = 0.0f;

        FTimelineTempoMarker() = default;
        explicit FTimelineTempoMarker(Readers::FArchive& Ar) : BaseGuid(Ar)
        {
            TimeSignature = Ar.Read<int64_t>();
            Position = Ar.Read<uint32_t>();
            Tempo = Ar.Read<float>();
        }
    };
}
