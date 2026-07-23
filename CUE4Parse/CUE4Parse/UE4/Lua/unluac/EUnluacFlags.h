// Ported from CUE4Parse/UE4/Lua/unluac/EUnluacFlags.cs
// match custom unluac native flags
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Lua::unluac
{
    // C# [Flags] enum. Kept unscoped-with-explicit-operators: scoped so the names stay namespaced, plus the
    // bitwise operators C# gets for free on [Flags] enums.
    enum class EUnluacFlags : uint32_t
    {
        None           = 0,
        RawString      = 1u << 0,
        Luaj           = 1u << 1,
        NoDebug        = 1u << 2,

        OpCodeMap      = 1u << 16,
        OpCodeMapPatch = 1u << 17,

        Decompile      = 1u << 24,
        Disassemble    = 1u << 25,
    };

    constexpr EUnluacFlags operator|(EUnluacFlags a, EUnluacFlags b)
    {
        return static_cast<EUnluacFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }
    constexpr EUnluacFlags operator&(EUnluacFlags a, EUnluacFlags b)
    {
        return static_cast<EUnluacFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }
    constexpr EUnluacFlags operator~(EUnluacFlags a)
    {
        return static_cast<EUnluacFlags>(~static_cast<uint32_t>(a));
    }
    constexpr EUnluacFlags& operator|=(EUnluacFlags& a, EUnluacFlags b) { a = a | b; return a; }
    constexpr EUnluacFlags& operator&=(EUnluacFlags& a, EUnluacFlags b) { a = a & b; return a; }

    // C#'s Enum.HasFlag.
    constexpr bool HasFlag(EUnluacFlags value, EUnluacFlags flag)
    {
        return (static_cast<uint32_t>(value) & static_cast<uint32_t>(flag)) == static_cast<uint32_t>(flag);
    }
}
