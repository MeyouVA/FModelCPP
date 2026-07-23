// Ported from CUE4Parse/UE4/FMod/Enums/EStringTableType.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::FMod::Enums
{
    enum class EStringTableType : uint32_t
    {
        StringTable_RadixTree_32Bit = 0x0,
        StringTable_RadixTree_24Bit = 0x1,
        StringTable_Max             = 0x2,
    };
}
