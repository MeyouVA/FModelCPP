// Ported from CUE4Parse/UE4/Wwise/Enums/Flags/EPriorityMidiFlags.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums::Flags
{
    // C# [Flags] enum: the bitwise operators C# gets for free are spelled out below.
    enum class EPriorityMidiFlags : uint8_t
    {
        None                       = 0,
        PriorityOverrideParent     = 1u << 0,
        PriorityApplyDistFactor    = 1u << 1,
        OverrideMidiEventsBehavior = 1u << 2,
        OverrideMidiNoteTracking   = 1u << 3,
        EnableMidiNoteTracking     = 1u << 4,
        MidiBreakLoopOnNoteOff     = 1u << 5,
    };

    constexpr EPriorityMidiFlags operator|(EPriorityMidiFlags a, EPriorityMidiFlags b)
    { return static_cast<EPriorityMidiFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b)); }
    constexpr EPriorityMidiFlags operator&(EPriorityMidiFlags a, EPriorityMidiFlags b)
    { return static_cast<EPriorityMidiFlags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b)); }
    constexpr EPriorityMidiFlags operator~(EPriorityMidiFlags a)
    { return static_cast<EPriorityMidiFlags>(static_cast<uint8_t>(~static_cast<uint8_t>(a))); }
    constexpr EPriorityMidiFlags& operator|=(EPriorityMidiFlags& a, EPriorityMidiFlags b) { a = a | b; return a; }
    constexpr EPriorityMidiFlags& operator&=(EPriorityMidiFlags& a, EPriorityMidiFlags b) { a = a & b; return a; }
    // C#'s Enum.HasFlag.
    constexpr bool HasFlag(EPriorityMidiFlags value, EPriorityMidiFlags flag)
    { return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) == static_cast<uint8_t>(flag); }
}
