// Ported from CUE4Parse/UE4/Wwise/Enums/Flags/EResumeOptionsFlags.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums::Flags
{
    // C# [Flags] enum: the bitwise operators C# gets for free are spelled out below.
    enum class EResumeOptionsFlags : uint8_t
    {
        None                    = 0,
        IsMasterResume          = 1u << 0,
        ApplyToStateTransitions = 1u << 1,
        ApplyToDynamicSequence  = 1u << 2,
    };

    constexpr EResumeOptionsFlags operator|(EResumeOptionsFlags a, EResumeOptionsFlags b)
    { return static_cast<EResumeOptionsFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b)); }
    constexpr EResumeOptionsFlags operator&(EResumeOptionsFlags a, EResumeOptionsFlags b)
    { return static_cast<EResumeOptionsFlags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b)); }
    constexpr EResumeOptionsFlags operator~(EResumeOptionsFlags a)
    { return static_cast<EResumeOptionsFlags>(static_cast<uint8_t>(~static_cast<uint8_t>(a))); }
    constexpr EResumeOptionsFlags& operator|=(EResumeOptionsFlags& a, EResumeOptionsFlags b) { a = a | b; return a; }
    constexpr EResumeOptionsFlags& operator&=(EResumeOptionsFlags& a, EResumeOptionsFlags b) { a = a & b; return a; }
    // C#'s Enum.HasFlag.
    constexpr bool HasFlag(EResumeOptionsFlags value, EResumeOptionsFlags flag)
    { return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) == static_cast<uint8_t>(flag); }
}
