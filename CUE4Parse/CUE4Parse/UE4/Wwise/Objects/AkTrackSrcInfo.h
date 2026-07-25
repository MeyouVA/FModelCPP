// Ported from CUE4Parse/UE4/Wwise/Objects/AkTrackSrcInfo.cs
#pragma once

#include <cstdint>

#include "../WwiseArchive.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    struct AkTrackSrcInfo
    {
        uint32_t TrackId = 0;
        uint32_t SourceId = 0;
        uint32_t CacheId = 0;
        uint32_t EventId = 0;
        double PlayAt = 0;
        double BeginTrimOffset = 0;
        double EndTrimOffset = 0;
        double SrcDuration = 0;

        AkTrackSrcInfo() = default;

        explicit AkTrackSrcInfo(FWwiseArchive& Ar)
        {
            TrackId = Ar.Read<uint32_t>();
            SourceId = Ar.Read<uint32_t>();

            if (Ar.Version > 150)
            {
                CacheId = Ar.Read<uint32_t>();
            }

            if (Ar.Version > 132)
            {
                EventId = Ar.Read<uint32_t>();
            }

            PlayAt = Ar.Read<double>();
            BeginTrimOffset = Ar.Read<double>();
            EndTrimOffset = Ar.Read<double>();
            SrcDuration = Ar.Read<double>();
        }
    };
}
