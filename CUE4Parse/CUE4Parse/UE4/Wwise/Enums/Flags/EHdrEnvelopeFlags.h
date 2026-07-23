// Ported from CUE4Parse/UE4/Wwise/Enums/Flags/EHdrEnvelopeFlags.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums::Flags
{
    // C# [Flags] enum: the bitwise operators C# gets for free are spelled out below.
    enum class EHdrEnvelopeFlags : uint8_t
    {
        None                = 0,
        OverrideHdrEnvelope = 1u << 0,
        OverrideAnalysis    = 1u << 1,
        NormalizeLoudness   = 1u << 2,
        EnableEnvelope      = 1u << 3,
    };

    constexpr EHdrEnvelopeFlags operator|(EHdrEnvelopeFlags a, EHdrEnvelopeFlags b)
    { return static_cast<EHdrEnvelopeFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b)); }
    constexpr EHdrEnvelopeFlags operator&(EHdrEnvelopeFlags a, EHdrEnvelopeFlags b)
    { return static_cast<EHdrEnvelopeFlags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b)); }
    constexpr EHdrEnvelopeFlags operator~(EHdrEnvelopeFlags a)
    { return static_cast<EHdrEnvelopeFlags>(static_cast<uint8_t>(~static_cast<uint8_t>(a))); }
    constexpr EHdrEnvelopeFlags& operator|=(EHdrEnvelopeFlags& a, EHdrEnvelopeFlags b) { a = a | b; return a; }
    constexpr EHdrEnvelopeFlags& operator&=(EHdrEnvelopeFlags& a, EHdrEnvelopeFlags b) { a = a & b; return a; }
    // C#'s Enum.HasFlag.
    constexpr bool HasFlag(EHdrEnvelopeFlags value, EHdrEnvelopeFlags flag)
    { return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) == static_cast<uint8_t>(flag); }
}
