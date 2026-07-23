// Ported from CUE4Parse/UE4/Wwise/Enums/EAKBKSourceType.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums
{
    // AkBank::AKBKSourceType
    enum class EAKBKSourceType : uint8_t
    {
        Data              = 0x0,
        PrefetchStreaming = 0x1,
        Streaming         = 0x2,
    };
}
