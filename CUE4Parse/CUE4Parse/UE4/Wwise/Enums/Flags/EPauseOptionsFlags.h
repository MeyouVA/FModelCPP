// Ported from CUE4Parse/UE4/Wwise/Enums/Flags/EPauseOptionsFlags.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums::Flags
{
    // C# [Flags] enum: the bitwise operators C# gets for free are spelled out below.
    enum class EPauseOptionsFlags : uint8_t
    {
        None                    = 0,
        PausePendingResume      = 1u << 0,
        ApplyToStateTransitions = 1u << 1,
        ApplyToDynamicSequence  = 1u << 2,
    };

    constexpr EPauseOptionsFlags operator|(EPauseOptionsFlags a, EPauseOptionsFlags b)
    { return static_cast<EPauseOptionsFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b)); }
    constexpr EPauseOptionsFlags operator&(EPauseOptionsFlags a, EPauseOptionsFlags b)
    { return static_cast<EPauseOptionsFlags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b)); }
    constexpr EPauseOptionsFlags operator~(EPauseOptionsFlags a)
    { return static_cast<EPauseOptionsFlags>(static_cast<uint8_t>(~static_cast<uint8_t>(a))); }
    constexpr EPauseOptionsFlags& operator|=(EPauseOptionsFlags& a, EPauseOptionsFlags b) { a = a | b; return a; }
    constexpr EPauseOptionsFlags& operator&=(EPauseOptionsFlags& a, EPauseOptionsFlags b) { a = a & b; return a; }
    // C#'s Enum.HasFlag.
    constexpr bool HasFlag(EPauseOptionsFlags value, EPauseOptionsFlags flag)
    { return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) == static_cast<uint8_t>(flag); }
}
