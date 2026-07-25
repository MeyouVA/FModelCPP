// Ported from CUE4Parse/UE4/Wwise/Objects/AkGameSync.cs
#pragma once

#include <cstdint>
#include <vector>

#include "../WwiseArchive.h"
#include "../Enums/EAkGroupType.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    using CUE4Parse::UE4::Wwise::Enums::EAkGroupType;

    struct AkGameSync
    {
        uint32_t GroupId = 0;
        EAkGroupType GroupType = static_cast<EAkGroupType>(0);

        AkGameSync() = default;
        AkGameSync(uint32_t groupId, EAkGroupType groupType) : GroupId(groupId), GroupType(groupType) {}

        // The two columns are stored one after the other, not interleaved -- hence the name.
        static std::vector<AkGameSync> ReadSequential(FWwiseArchive& Ar, uint32_t count)
        {
            auto groupIds = Ar.ReadArray<uint32_t>(static_cast<int>(count));
            auto groupTypes = Ar.ReadArray<EAkGroupType>(static_cast<int>(count));

            std::vector<AkGameSync> gameSyncs;
            gameSyncs.reserve(count);
            for (uint32_t i = 0; i < count; i++)
                gameSyncs.emplace_back(groupIds[i], groupTypes[i]);

            return gameSyncs;
        }
    };
}
