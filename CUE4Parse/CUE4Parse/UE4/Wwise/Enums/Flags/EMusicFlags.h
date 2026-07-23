// Ported from CUE4Parse/UE4/Wwise/Enums/Flags/EMusicFlags.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums::Flags
{
    // C# [Flags] enum: the bitwise operators C# gets for free are spelled out below.
    enum class EMusicFlags : uint8_t
    {
        None                     = 0,
        OverrideParentMidiTempo  = 1u << 1,
        OverrideParentMidiTarget = 1u << 2,
        MidiTargetTypeBus        = 1u << 3, // (only present in v113–v152)
    };

    constexpr EMusicFlags operator|(EMusicFlags a, EMusicFlags b)
    { return static_cast<EMusicFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b)); }
    constexpr EMusicFlags operator&(EMusicFlags a, EMusicFlags b)
    { return static_cast<EMusicFlags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b)); }
    constexpr EMusicFlags operator~(EMusicFlags a)
    { return static_cast<EMusicFlags>(static_cast<uint8_t>(~static_cast<uint8_t>(a))); }
    constexpr EMusicFlags& operator|=(EMusicFlags& a, EMusicFlags b) { a = a | b; return a; }
    constexpr EMusicFlags& operator&=(EMusicFlags& a, EMusicFlags b) { a = a & b; return a; }
    // C#'s Enum.HasFlag.
    constexpr bool HasFlag(EMusicFlags value, EMusicFlags flag)
    { return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) == static_cast<uint8_t>(flag); }
}
