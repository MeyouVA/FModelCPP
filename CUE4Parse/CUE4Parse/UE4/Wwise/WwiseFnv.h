// Ported from CUE4Parse/UE4/Wwise/WwiseFnv.cs
// FNV-1 (not 1a) over the *lowercased* UTF-8 name -- the hash Wwise uses for every event, bus and
// game-sync ID in a bank.
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace CUE4Parse::UE4::Wwise::WwiseFnv
{
    // C#'s private ComputeHash(byte[]). Note the multiply comes *before* the xor, which is what makes
    // this FNV-1 rather than the more common FNV-1a.
    constexpr uint32_t ComputeHash(std::string_view nameBytes)
    {
        uint32_t hash = 2166136261u; // FNV offset basis
        for (const char c : nameBytes)
        {
            hash *= 16777619u;                              // FNV prime
            hash ^= static_cast<uint8_t>(c);
                                                            // C#'s `hash &= 0xFFFFFFFF` is a no-op on uint
        }
        return hash;
    }

    constexpr uint32_t GetHashLower(std::string_view lowerName)
    {
        return ComputeHash(lowerName);
    }

    // C# calls string.ToLowerInvariant(). Wwise IDs are ASCII, so an ASCII-only fold matches it for
    // every name that can actually appear in a bank without dragging in a locale-aware lowercase.
    inline uint32_t GetHash(std::string_view name)
    {
        std::string lower(name);
        for (char& c : lower)
            if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
        return GetHashLower(lower);
    }
}
