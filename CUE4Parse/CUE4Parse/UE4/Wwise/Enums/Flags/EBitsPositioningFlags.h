// Ported from CUE4Parse/UE4/Wwise/Enums/Flags/EBitsPositioningFlags.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums::Flags
{
    // C# [Flags] enum: the bitwise operators C# gets for free are spelled out below.
    enum class EBitsPositioningFlags : uint8_t
    {
        PositioningInfoOverrideParent = 1u << 0,
        HasListenerRelativeRouting    = 1u << 1,
        // v112+
        Unknown2d_2                   = 1u << 2,
        Unknown2d_3                   = 1u << 3,
        Unknown3d_4                   = 1u << 4,
        Unknown3d_5                   = 1u << 5,
        Unknown3d_6                   = 1u << 6,
        Unknown3d_7                   = 1u << 7,
        // v122+
        // Unknown2d_1 = 1 << 1, // overlaps HasListenerRelativeRouting
        Is3DPositioningAvailable_122  = 1u << 3,
        // v129+
        Is3DPositioningAvailable_129  = 1u << 4,
        // v130+
        PannerTypeMask                = 0b0000'1100,
        PositionTypeMask              = 0b0110'0000,
    };

    constexpr EBitsPositioningFlags operator|(EBitsPositioningFlags a, EBitsPositioningFlags b)
    { return static_cast<EBitsPositioningFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b)); }
    constexpr EBitsPositioningFlags operator&(EBitsPositioningFlags a, EBitsPositioningFlags b)
    { return static_cast<EBitsPositioningFlags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b)); }
    constexpr EBitsPositioningFlags operator~(EBitsPositioningFlags a)
    { return static_cast<EBitsPositioningFlags>(static_cast<uint8_t>(~static_cast<uint8_t>(a))); }
    constexpr EBitsPositioningFlags& operator|=(EBitsPositioningFlags& a, EBitsPositioningFlags b) { a = a | b; return a; }
    constexpr EBitsPositioningFlags& operator&=(EBitsPositioningFlags& a, EBitsPositioningFlags b) { a = a & b; return a; }
    // C#'s Enum.HasFlag.
    constexpr bool HasFlag(EBitsPositioningFlags value, EBitsPositioningFlags flag)
    { return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) == static_cast<uint8_t>(flag); }
}
