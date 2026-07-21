// Ported from CUE4Parse/Utils/UnsafePrint.cs
// Uppercase hex formatting used by the color types' Hex property. The C# `unsafe byte*` overload and the
// `params byte[]` (BitConverter.ToString) overload collapse into a pointer+length function plus an
// initializer_list convenience here.
#pragma once

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string>

namespace CUE4Parse::Utils::UnsafePrint
{
    inline std::string BytesToHex(const uint8_t* bytes, std::size_t length)
    {
        static constexpr char kDigits[] = "0123456789ABCDEF";
        std::string c(length * 2, '\0');
        for (std::size_t bx = 0, cx = 0; bx < length; ++bx)
        {
            c[cx++] = kDigits[(bytes[bx] >> 4) & 0x0F];
            c[cx++] = kDigits[bytes[bx] & 0x0F];
        }
        return c;
    }

    inline std::string BytesToHex(std::initializer_list<uint8_t> bytes)
    {
        return BytesToHex(bytes.begin(), bytes.size());
    }
}
