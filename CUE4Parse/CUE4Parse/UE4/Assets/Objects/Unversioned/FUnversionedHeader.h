// Ported from CUE4Parse/UE4/Assets/Objects/Unversioned/FUnversionedHeader.cs
// The header preceding an unversioned property blob: a fragment list plus the zero-mask bits for the
// fragments that flagged HasAnyZeroes.
//
// Deliberate difference from C#: the .NET BitArray becomes std::vector<bool> with the same LSB-first bit
// order (bit i of byte b -> index b*8+i; the <=16-bit forms read 1/2 bytes, larger ones whole int32s).
#pragma once

#include <cstdint>
#include <vector>

#include "FFragment.h"

namespace CUE4Parse::UE4::Readers { class FArchive; }

namespace CUE4Parse::UE4::Assets::Objects::Unversioned
{
    class FUnversionedHeader
    {
    public:
        std::vector<FFragment> Fragments;
        std::vector<bool> ZeroMask;
        bool HasNonZeroValues = false;

        bool HasValues() const { return HasNonZeroValues || !ZeroMask.empty(); }

        explicit FUnversionedHeader(Readers::FArchive& Ar);

        // C#'s BitArray.GetOrFalse: out-of-range indices read as false.
        bool ZeroMaskGetOrFalse(size_t index) const
        {
            return index < ZeroMask.size() && ZeroMask[index];
        }

    private:
        static void LoadZeroMaskData(Readers::FArchive& Ar, int numBits, std::vector<bool>& data);
    };
}
