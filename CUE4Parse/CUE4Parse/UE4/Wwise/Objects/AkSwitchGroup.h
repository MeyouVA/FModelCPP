// Ported from CUE4Parse/UE4/Wwise/Objects/AkSwitchGroup.cs
#pragma once

#include <cstdint>
#include <vector>

#include "../WwiseArchive.h"
#include "../Enums/EAkGameSyncType.h"
#include "AkConversionTable.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    using CUE4Parse::UE4::Wwise::Enums::EAkGameSyncType;

    struct AkSwitchGroup
    {
        uint32_t SwitchGroupId = 0;
        uint32_t RtpcId = 0;
        EAkGameSyncType RtpcType = static_cast<EAkGameSyncType>(0);
        std::vector<AkSwitchGraphPoint> GraphPoints;

        AkSwitchGroup() = default;

        explicit AkSwitchGroup(FWwiseArchive& Ar)
        {
            SwitchGroupId = Ar.Read<uint32_t>();
            RtpcId = Ar.Read<uint32_t>();
            if (Ar.Version > 89)
                RtpcType = Ar.Read<EAkGameSyncType>();
            const int count = static_cast<int>(Ar.Read<uint32_t>());
            GraphPoints = Ar.ReadArrayWith(count, [&Ar] { return AkSwitchGraphPoint(Ar); });
        }
    };
}
