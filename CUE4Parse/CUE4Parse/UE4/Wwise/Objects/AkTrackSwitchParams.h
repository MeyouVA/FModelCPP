// Ported from CUE4Parse/UE4/Wwise/Objects/AkTrackSwitchParams.cs
#pragma once

#include <cstdint>
#include <vector>

#include "../WwiseArchive.h"
#include "../Enums/EAkGroupType.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    using CUE4Parse::UE4::Wwise::Enums::EAkGroupType;

    class AkTrackSwitchParams
    {
    public:
        EAkGroupType GroupType = static_cast<EAkGroupType>(0);
        uint32_t GroupId = 0;
        uint32_t DefaultSwitch = 0;
        std::vector<uint32_t> SwitchAssociationIds;

        AkTrackSwitchParams() = default;

        explicit AkTrackSwitchParams(FWwiseArchive& Ar)
        {
            GroupType = Ar.Read<EAkGroupType>();
            GroupId = Ar.Read<uint32_t>();
            DefaultSwitch = Ar.Read<uint32_t>();
            SwitchAssociationIds = Ar.ReadArray<uint32_t>(static_cast<int>(Ar.Read<uint32_t>()));
        }
    };
}
