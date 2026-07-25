// Ported from CUE4Parse/UE4/Wwise/Objects/AkMediaMap.cs
#pragma once

#include <cstdint>

#include "../WwiseArchive.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    struct AkMediaMap
    {
        uint8_t Index = 0;
        uint32_t SourceId = 0;

        AkMediaMap() = default;

        explicit AkMediaMap(FWwiseArchive& Ar)
        {
            Index = Ar.Read<uint8_t>();
            SourceId = Ar.Read<uint32_t>();
        }
    };
}
