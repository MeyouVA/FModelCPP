// Ported from CUE4Parse/UE4/FMod/Objects/FRoutable.cs
#pragma once

#include "FModGuid.h"

namespace CUE4Parse::UE4::FMod::Objects
{
    struct FRoutable
    {
        FModGuid BaseGuid;
        uint32_t OutputChannelLayout = 0;
        uint32_t ChannelMask = 0;

        FRoutable() = default;
        explicit FRoutable(Readers::FArchive& Ar)
        {
            (void) Ar.Read<int16_t>(); // Payload size
            BaseGuid = FModGuid(Ar);
            OutputChannelLayout = Ar.Read<uint32_t>();
            ChannelMask = Ar.Read<uint32_t>();
        }
    };
}
