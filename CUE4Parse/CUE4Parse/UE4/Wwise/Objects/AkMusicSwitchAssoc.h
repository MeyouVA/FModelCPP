// Ported from CUE4Parse/UE4/Wwise/Objects/AkMusicSwitchAssoc.cs
#pragma once

#include <cstdint>

#include "../WwiseArchive.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    struct AkMusicSwitchAssoc
    {
        uint32_t SwitchId = 0;
        uint32_t NodeId = 0;

        AkMusicSwitchAssoc() = default;

        explicit AkMusicSwitchAssoc(FWwiseArchive& Ar)
        {
            SwitchId = Ar.Read<uint32_t>();
            NodeId = Ar.Read<uint32_t>();
        }
    };
}
