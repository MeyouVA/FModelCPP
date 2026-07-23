// Ported from CUE4Parse/UE4/Wwise/Enums/Flags/EPlaylistFlags.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums::Flags
{
    // C# [Flags] enum: the bitwise operators C# gets for free are spelled out below.
    enum class EPlayListFlags : uint8_t
    {
        None                    = 0,
        IsUsingWeight           = 1u << 0,
        ResetPlayListAtEachPlay = 1u << 1,
        IsRestartBackward       = 1u << 2,
        IsContinuous            = 1u << 3,
        IsGlobal                = 1u << 4,
    };

    constexpr EPlayListFlags operator|(EPlayListFlags a, EPlayListFlags b)
    { return static_cast<EPlayListFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b)); }
    constexpr EPlayListFlags operator&(EPlayListFlags a, EPlayListFlags b)
    { return static_cast<EPlayListFlags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b)); }
    constexpr EPlayListFlags operator~(EPlayListFlags a)
    { return static_cast<EPlayListFlags>(static_cast<uint8_t>(~static_cast<uint8_t>(a))); }
    constexpr EPlayListFlags& operator|=(EPlayListFlags& a, EPlayListFlags b) { a = a | b; return a; }
    constexpr EPlayListFlags& operator&=(EPlayListFlags& a, EPlayListFlags b) { a = a & b; return a; }
    // C#'s Enum.HasFlag.
    constexpr bool HasFlag(EPlayListFlags value, EPlayListFlags flag)
    { return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) == static_cast<uint8_t>(flag); }
}
