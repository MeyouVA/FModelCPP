// Ported from CUE4Parse/Utils/ArrayUtils.cs
//
// C#'s BitArray maps to std::vector<bool> here. The C# Trace.Assert calls are dropped (the callers
// are expected to pass valid ranges, as in C# release builds).
#pragma once

#include <cstdint>
#include <vector>

namespace CUE4Parse::Utils
{
    inline std::vector<uint8_t> SubByteArray(const std::vector<uint8_t>& byteArray, int len)
    {
        return std::vector<uint8_t>(byteArray.begin(), byteArray.begin() + len);
    }

    inline bool Contains(const std::vector<bool>& array, bool search)
    {
        for (size_t i = 0; i < array.size(); i++)
        {
            if (array[i] == search)
                return true;
        }
        return false;
    }

    inline bool GetOrFalse(const std::vector<bool>& array, int index)
    {
        return index >= 0 && index < static_cast<int>(array.size()) && array[index];
    }

    inline void SetRangeFromRange(std::vector<bool>& array, int index, int numBitsToSet,
                                  const std::vector<bool>& readBits, int readOffsetBits = 0)
    {
        for (int i = 0; i < numBitsToSet; i++)
        {
            array[index + i] = readBits[readOffsetBits + i];
        }
    }
}
