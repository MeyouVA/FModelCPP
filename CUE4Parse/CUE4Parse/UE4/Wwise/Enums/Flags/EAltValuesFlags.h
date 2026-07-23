// Ported from CUE4Parse/UE4/Wwise/Enums/Flags/EAltValuesFlags.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums::Flags
{
    // C# [Flags] enum: the bitwise operators C# gets for free are spelled out below.
    enum class EAltValuesFlags : uint32_t
    {
        None             = 0x0,
        UAlignment       = 0x10,
        BDeviceAllocated = 0x10000,
    };

    constexpr EAltValuesFlags operator|(EAltValuesFlags a, EAltValuesFlags b)
    { return static_cast<EAltValuesFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b)); }
    constexpr EAltValuesFlags operator&(EAltValuesFlags a, EAltValuesFlags b)
    { return static_cast<EAltValuesFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b)); }
    constexpr EAltValuesFlags operator~(EAltValuesFlags a)
    { return static_cast<EAltValuesFlags>(static_cast<uint32_t>(~static_cast<uint32_t>(a))); }
    constexpr EAltValuesFlags& operator|=(EAltValuesFlags& a, EAltValuesFlags b) { a = a | b; return a; }
    constexpr EAltValuesFlags& operator&=(EAltValuesFlags& a, EAltValuesFlags b) { a = a & b; return a; }
    // C#'s Enum.HasFlag.
    constexpr bool HasFlag(EAltValuesFlags value, EAltValuesFlags flag)
    { return (static_cast<uint32_t>(value) & static_cast<uint32_t>(flag)) == static_cast<uint32_t>(flag); }
}
