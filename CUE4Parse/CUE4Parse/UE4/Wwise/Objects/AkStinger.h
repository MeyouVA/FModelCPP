// Ported from CUE4Parse/UE4/Wwise/Objects/AkStinger.cs
#pragma once

#include <cstdint>
#include <vector>

#include "../WwiseArchive.h"
#include "../Enums/EAkSyncType.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    using CUE4Parse::UE4::Wwise::Enums::EAkSyncType;

    struct AkStinger
    {
        uint32_t TriggerId = 0;
        uint32_t SegmentId = 0;
        EAkSyncType SyncPlayAt = static_cast<EAkSyncType>(0);
        uint32_t CueFilterHash = 0;
        int32_t DontRepeatTime = 0;
        uint32_t NumSegmentLookAhead = 0;

        AkStinger() = default;

        explicit AkStinger(FWwiseArchive& Ar)
        {
            TriggerId = Ar.Read<uint32_t>();
            SegmentId = Ar.Read<uint32_t>();
            SyncPlayAt = Ar.Read<EAkSyncType>();

            if (Ar.Version > 62)
            {
                CueFilterHash = Ar.Read<uint32_t>();
            }

            DontRepeatTime = Ar.Read<int32_t>();
            NumSegmentLookAhead = Ar.Read<uint32_t>();
        }

        static std::vector<AkStinger> ReadArray(FWwiseArchive& Ar)
        {
            const int count = static_cast<int>(Ar.Read<uint32_t>());
            return Ar.ReadArrayWith(count, [&Ar] { return AkStinger(Ar); });
        }
    };
}
