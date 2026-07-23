// Ported from CUE4Parse/UE4/Wwise/Enums/Flags/EAkHdrEnvelopeFlags.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums::Flags
{
    // C# [Flags] enum: the bitwise operators C# gets for free are spelled out below.
    // C# tags this [JsonConverter(typeof(StringEnumConverter))] -- it serialises by member
    // name, not by number. The JSON writer is not ported yet; noted here so it is not lost.
    enum class EAkHdrEnvelopeFlags : uint8_t
    {
        None                = 0,
        OverrideHdrEnvelope = 1u << 0,
        OverrideAnalysis    = 1u << 1,
        NormalizeLoudness   = 1u << 2,
        EnableEnvelope      = 1u << 3,
    };

    constexpr EAkHdrEnvelopeFlags operator|(EAkHdrEnvelopeFlags a, EAkHdrEnvelopeFlags b)
    { return static_cast<EAkHdrEnvelopeFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b)); }
    constexpr EAkHdrEnvelopeFlags operator&(EAkHdrEnvelopeFlags a, EAkHdrEnvelopeFlags b)
    { return static_cast<EAkHdrEnvelopeFlags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b)); }
    constexpr EAkHdrEnvelopeFlags operator~(EAkHdrEnvelopeFlags a)
    { return static_cast<EAkHdrEnvelopeFlags>(static_cast<uint8_t>(~static_cast<uint8_t>(a))); }
    constexpr EAkHdrEnvelopeFlags& operator|=(EAkHdrEnvelopeFlags& a, EAkHdrEnvelopeFlags b) { a = a | b; return a; }
    constexpr EAkHdrEnvelopeFlags& operator&=(EAkHdrEnvelopeFlags& a, EAkHdrEnvelopeFlags b) { a = a & b; return a; }
    // C#'s Enum.HasFlag.
    constexpr bool HasFlag(EAkHdrEnvelopeFlags value, EAkHdrEnvelopeFlags flag)
    { return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) == static_cast<uint8_t>(flag); }
}
