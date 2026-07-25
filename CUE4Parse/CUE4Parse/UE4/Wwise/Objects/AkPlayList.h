// Ported from CUE4Parse/UE4/Wwise/Objects/AkPlayList.cs
#pragma once

#include <cstdint>
#include <vector>

#include "../WwiseArchive.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    struct AkPlayListItem
    {
        uint32_t PlayId = 0;
        int32_t Weight = 0;

        AkPlayListItem() = default;

        explicit AkPlayListItem(FWwiseArchive& Ar)
        {
            PlayId = Ar.Read<uint32_t>();
            Weight = Ar.Version <= 56 ? Ar.Read<uint8_t>() : Ar.Read<int32_t>(); // Could also be uint for version 128
        }
    };

    struct AkPlayList
    {
        std::vector<AkPlayListItem> PlaylistItems;

        AkPlayList() = default;

        explicit AkPlayList(FWwiseArchive& Ar)
        {
            const uint32_t numItems = Ar.Version <= 38 ? Ar.Read<uint32_t>() : Ar.Read<uint16_t>();
            PlaylistItems = Ar.ReadArrayWith(static_cast<int>(numItems), [&Ar] { return AkPlayListItem(Ar); });
        }
    };
}
