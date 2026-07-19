// Ported from CUE4Parse/Utils/HexUtils.cs
#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace CUE4Parse::Utils
{
    namespace detail
    {
        inline int GetHexVal(char hex)
        {
            int val = static_cast<unsigned char>(hex);
            // Combined upper/lowercase table: '0'-'9' -> 0-9, 'A'-'F'/'a'-'f' -> 10-15.
            return val - (val < 58 ? 48 : (val < 97 ? 55 : 87));
        }
    }

    inline std::vector<uint8_t> ParseHexBinary(const std::string& hex)
    {
        if (hex.size() % 2 == 1)
            throw std::invalid_argument("The binary key cannot have an odd number of digits");

        std::vector<uint8_t> arr(hex.size() >> 1);
        for (size_t i = 0; i < (hex.size() >> 1); ++i)
        {
            arr[i] = static_cast<uint8_t>((detail::GetHexVal(hex[i << 1]) << 4) + detail::GetHexVal(hex[(i << 1) + 1]));
        }
        return arr;
    }
}
