// Ported from CUE4Parse/UE4/Assets/Objects/Unversioned/FUnversionedHeader.cs
#include "FUnversionedHeader.h"

#include <algorithm>

#include "../../../Readers/FArchive.h"

namespace CUE4Parse::UE4::Assets::Objects::Unversioned
{
    using Readers::FArchive;

    FUnversionedHeader::FUnversionedHeader(FArchive& Ar)
    {
        FFragment fragment;
        int zeroMaskNum = 0;
        uint32_t unmaskedNum = 0;

        do
        {
            fragment = FFragment(Ar.Read<uint16_t>());
            Fragments.push_back(fragment);

            if (fragment.HasAnyZeroes)
                zeroMaskNum += fragment.ValueNum;
            else
                unmaskedNum += fragment.ValueNum;
        } while (!fragment.IsLast);

        if (zeroMaskNum > 0)
        {
            LoadZeroMaskData(Ar, zeroMaskNum, ZeroMask);
            HasNonZeroValues = unmaskedNum > 0 ||
                std::find(ZeroMask.begin(), ZeroMask.end(), false) != ZeroMask.end();
        }
        else
        {
            HasNonZeroValues = unmaskedNum > 0;
        }
    }

    void FUnversionedHeader::LoadZeroMaskData(FArchive& Ar, int numBits, std::vector<bool>& data)
    {
        // C# builds a BitArray over 1 byte / 2 bytes / ceil(numBits/32) ints, then truncates to numBits.
        int byteCount;
        if (numBits <= 8) byteCount = 1;
        else if (numBits <= 16) byteCount = 2;
        else byteCount = ((numBits + 31) / 32) * 4;

        const auto bytes = Ar.ReadBytes(byteCount);
        data.resize(static_cast<size_t>(numBits));
        for (int i = 0; i < numBits; i++)
            data[static_cast<size_t>(i)] = (bytes[static_cast<size_t>(i / 8)] >> (i % 8) & 1) != 0;
    }
}
