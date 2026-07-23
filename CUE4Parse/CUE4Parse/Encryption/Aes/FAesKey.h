// Ported from CUE4Parse/Encryption/Aes/FAesKey.cs
// A 256-bit AES key, constructed either from raw bytes or from a hex string ("0x"-prefixed or bare).
#pragma once

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../../Utils/HexUtils.h"

namespace CUE4Parse::Encryption::Aes
{
    class FAesKey
    {
    public:
        // C#'s `public readonly byte[] Key`. Kept public and const-in-spirit (assigned only by the ctors).
        std::vector<uint8_t> Key;

        explicit FAesKey(std::vector<uint8_t> key, bool ignoreLength = false) : Key(std::move(key))
        {
            if (!ignoreLength && Key.size() != 32)
                throw std::invalid_argument("Aes Key must be 32 bytes long");
        }

        explicit FAesKey(const std::string& keyString)
        {
            if (keyString.rfind("0x", 0) == 0 && keyString.size() == 66)
                Key = Utils::ParseHexBinary(keyString.substr(2));
            else if (keyString.size() == 64)
                Key = Utils::ParseHexBinary(keyString);
            else
                throw std::invalid_argument("Aes Key must be 32 bytes long");
        }

        // C#'s KeyString property. Convert.ToHexString is uppercase, so this is too.
        std::string KeyString() const
        {
            static const char* hex = "0123456789ABCDEF";
            std::string out = "0x";
            out.reserve(2 + Key.size() * 2);
            for (uint8_t b : Key)
            {
                out.push_back(hex[b >> 4]);
                out.push_back(hex[b & 0x0F]);
            }
            return out;
        }

        bool IsDefault() const
        {
            return std::all_of(Key.begin(), Key.end(), [](uint8_t x) { return x == 0; });
        }

        std::string ToString() const { return KeyString(); }
    };
}
