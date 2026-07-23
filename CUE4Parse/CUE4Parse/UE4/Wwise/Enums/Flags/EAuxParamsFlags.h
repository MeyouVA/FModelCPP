// Ported from CUE4Parse/UE4/Wwise/Enums/Flags/EAuxParamsFlags.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums::Flags
{
    // C# [Flags] enum: the bitwise operators C# gets for free are spelled out below.
    enum class EAuxParamsFlags : uint8_t
    {
        None                 = 0,
        OverrideGameAux      = 1u << 0,
        OverridePriority     = 1u << 1,
        OverrideUserAuxSends = 1u << 2,
        HasAux               = 1u << 3,
        OverrideReflections  = 1u << 4,
    };

    constexpr EAuxParamsFlags operator|(EAuxParamsFlags a, EAuxParamsFlags b)
    { return static_cast<EAuxParamsFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b)); }
    constexpr EAuxParamsFlags operator&(EAuxParamsFlags a, EAuxParamsFlags b)
    { return static_cast<EAuxParamsFlags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b)); }
    constexpr EAuxParamsFlags operator~(EAuxParamsFlags a)
    { return static_cast<EAuxParamsFlags>(static_cast<uint8_t>(~static_cast<uint8_t>(a))); }
    constexpr EAuxParamsFlags& operator|=(EAuxParamsFlags& a, EAuxParamsFlags b) { a = a | b; return a; }
    constexpr EAuxParamsFlags& operator&=(EAuxParamsFlags& a, EAuxParamsFlags b) { a = a & b; return a; }
    // C#'s Enum.HasFlag.
    constexpr bool HasFlag(EAuxParamsFlags value, EAuxParamsFlags flag)
    { return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) == static_cast<uint8_t>(flag); }
}
