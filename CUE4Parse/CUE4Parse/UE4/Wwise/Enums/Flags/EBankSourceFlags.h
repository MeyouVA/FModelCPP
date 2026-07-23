// Ported from CUE4Parse/UE4/Wwise/Enums/Flags/EBankSourceFlags.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums::Flags
{
    // > 112
    // C# [Flags] enum: the bitwise operators C# gets for free are spelled out below.
    enum class EBankSourceFlags : uint8_t
    {
        None               = 0,
        IsLanguageSpecific = 1u << 0,
        Prefetch           = 1u << 1,
        ExternallySupplied = 1u << 2, // Legacy, check below
        NonCachable        = 1u << 3,
        HasSource          = 1u << 7,
    };

    constexpr EBankSourceFlags operator|(EBankSourceFlags a, EBankSourceFlags b)
    { return static_cast<EBankSourceFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b)); }
    constexpr EBankSourceFlags operator&(EBankSourceFlags a, EBankSourceFlags b)
    { return static_cast<EBankSourceFlags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b)); }
    constexpr EBankSourceFlags operator~(EBankSourceFlags a)
    { return static_cast<EBankSourceFlags>(static_cast<uint8_t>(~static_cast<uint8_t>(a))); }
    constexpr EBankSourceFlags& operator|=(EBankSourceFlags& a, EBankSourceFlags b) { a = a | b; return a; }
    constexpr EBankSourceFlags& operator&=(EBankSourceFlags& a, EBankSourceFlags b) { a = a & b; return a; }
    // C#'s Enum.HasFlag.
    constexpr bool HasFlag(EBankSourceFlags value, EBankSourceFlags flag)
    { return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) == static_cast<uint8_t>(flag); }

    // <= 112
    // C# [Flags] enum: the bitwise operators C# gets for free are spelled out below.
    enum class EBankSourceFlags_v112 : uint8_t
    {
        None               = 0,
        IsLanguageSpecific = 1u << 0,
        HasSource          = 1u << 1,
        ExternallySupplied = 1u << 2,
    };

    constexpr EBankSourceFlags_v112 operator|(EBankSourceFlags_v112 a, EBankSourceFlags_v112 b)
    { return static_cast<EBankSourceFlags_v112>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b)); }
    constexpr EBankSourceFlags_v112 operator&(EBankSourceFlags_v112 a, EBankSourceFlags_v112 b)
    { return static_cast<EBankSourceFlags_v112>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b)); }
    constexpr EBankSourceFlags_v112 operator~(EBankSourceFlags_v112 a)
    { return static_cast<EBankSourceFlags_v112>(static_cast<uint8_t>(~static_cast<uint8_t>(a))); }
    constexpr EBankSourceFlags_v112& operator|=(EBankSourceFlags_v112& a, EBankSourceFlags_v112 b) { a = a | b; return a; }
    constexpr EBankSourceFlags_v112& operator&=(EBankSourceFlags_v112& a, EBankSourceFlags_v112 b) { a = a & b; return a; }
    // C#'s Enum.HasFlag.
    constexpr bool HasFlag(EBankSourceFlags_v112 value, EBankSourceFlags_v112 flag)
    { return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) == static_cast<uint8_t>(flag); }

    // C#'s BankSourceFlagsExtensions.MapToCurrent. Hand-written (the generator skips extension classes).
    // The two enums disagree on the bit layout, so this is a re-mapping and not a cast: v112's HasSource is
    // bit 1 while the current enum puts it at bit 7. Note C# does NOT map anything onto Prefetch/NonCachable,
    // which have no v112 counterpart, so they always come back clear.
    constexpr EBankSourceFlags MapToCurrent(EBankSourceFlags_v112 legacy)
    {
        auto current = EBankSourceFlags::None;

        if (HasFlag(legacy, EBankSourceFlags_v112::IsLanguageSpecific))
            current |= EBankSourceFlags::IsLanguageSpecific;
        if (HasFlag(legacy, EBankSourceFlags_v112::HasSource))
            current |= EBankSourceFlags::HasSource;
        if (HasFlag(legacy, EBankSourceFlags_v112::ExternallySupplied))
            current |= EBankSourceFlags::ExternallySupplied;

        return current;
    }
}
