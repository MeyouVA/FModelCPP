// Ported from CUE4Parse/UE4/Assets/Objects/Unversioned/FFragment.cs
// One packed ushort of an unversioned header: skip N schema slots, then M serialized values, with flags
// for "some of those values are zero-masked" and "last fragment".
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Assets::Objects::Unversioned
{
    struct FFragment
    {
        static constexpr uint32_t SkipMax = 127;
        static constexpr uint32_t ValueMax = 127;

        static constexpr uint32_t SkipNumMask = 0x007fu;
        static constexpr uint32_t HasZeroMask = 0x0080u;
        static constexpr int ValueNumShift = 9;
        static constexpr uint32_t IsLastMask = 0x0100u;

        uint8_t SkipNum = 0;      // Number of properties to skip before values
        bool HasAnyZeroes = false;
        uint8_t ValueNum = 0;     // Number of subsequent property values stored
        bool IsLast = false;      // Is this the last fragment of the header?

        FFragment() = default;

        explicit FFragment(uint16_t packed)
            : SkipNum(static_cast<uint8_t>(packed & SkipNumMask)),
              HasAnyZeroes((packed & HasZeroMask) != 0),
              ValueNum(static_cast<uint8_t>(packed >> ValueNumShift)),
              IsLast((packed & IsLastMask) != 0)
        {
        }
    };
}
