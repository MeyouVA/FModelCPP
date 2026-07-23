// Ported from CUE4Parse/UE4/Wwise/Enums/Flags/EAkAdvSettingsFlags.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums::Flags
{
    // C# [Flags] enum: the bitwise operators C# gets for free are spelled out below.
    // C# tags this [JsonConverter(typeof(StringEnumConverter))] -- it serialises by member
    // name, not by number. The JSON writer is not ported yet; noted here so it is not lost.
    enum class EAkAdvSettingsFlags : uint8_t
    {
        None                       = 0,
        KillNewest                 = 1u << 0,
        UseVirtualBehavior         = 1u << 1,
        IgnoreParentMaxNumInst     = 1u << 3,
        IsVVoicesOptOverrideParent = 1u << 4,
        IsMaxNumInstOverrideParent = IgnoreParentMaxNumInst,
    };

    constexpr EAkAdvSettingsFlags operator|(EAkAdvSettingsFlags a, EAkAdvSettingsFlags b)
    { return static_cast<EAkAdvSettingsFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b)); }
    constexpr EAkAdvSettingsFlags operator&(EAkAdvSettingsFlags a, EAkAdvSettingsFlags b)
    { return static_cast<EAkAdvSettingsFlags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b)); }
    constexpr EAkAdvSettingsFlags operator~(EAkAdvSettingsFlags a)
    { return static_cast<EAkAdvSettingsFlags>(static_cast<uint8_t>(~static_cast<uint8_t>(a))); }
    constexpr EAkAdvSettingsFlags& operator|=(EAkAdvSettingsFlags& a, EAkAdvSettingsFlags b) { a = a | b; return a; }
    constexpr EAkAdvSettingsFlags& operator&=(EAkAdvSettingsFlags& a, EAkAdvSettingsFlags b) { a = a & b; return a; }
    // C#'s Enum.HasFlag.
    constexpr bool HasFlag(EAkAdvSettingsFlags value, EAkAdvSettingsFlags flag)
    { return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) == static_cast<uint8_t>(flag); }
}
