// Ported from CUE4Parse/UE4/Objects/Core/Misc/FGuid.cs
// 16-byte globally unique identifier (four little-endian uint32s). Laid out as a standard-layout,
// trivially copyable value type so it can be read directly via FArchive::Read<FGuid>().
#pragma once

#include <array>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>

#include "../../../IUStruct.h"
#include "../Math/FUInt128.h"

namespace CUE4Parse::UE4::Objects::Core::Misc
{
    enum class EGuidFormats
    {
        Digits,                          // "00000000000000000000000000000000"
        DigitsWithHyphens,               // 00000000-0000-0000-0000-000000000000
        DigitsWithHyphensInBraces,       // {00000000-0000-0000-0000-000000000000}
        DigitsWithHyphensInParentheses,  // (00000000-0000-0000-0000-000000000000)
        HexValuesInBraces,               // {0x00000000,0x0000,0x0000,{0x00,...}}
        UniqueObjectGuid,                // 00000000-00000000-00000000-00000000
        Short,                           // AQsMCQ0PAAUKCgQEBAgADQ
        Base36Encoded,                   // 1DPF6ARFCM4XH5RMWPU8TGR0J
    };

    struct FGuid : public UE4::IUStruct
    {
        static constexpr int Size = sizeof(uint32_t) * 4;

        uint32_t A = 0;
        uint32_t B = 0;
        uint32_t C = 0;
        uint32_t D = 0;

        FGuid() = default;
        explicit FGuid(uint32_t v) : A(v), B(v), C(v), D(v) {}
        FGuid(uint32_t a, uint32_t b, uint32_t c, uint32_t d) : A(a), B(b), C(c), D(d) {}

        // Parse 32 hex characters (8 per component). Mirrors the ReadOnlySpan<char> constructor.
        explicit FGuid(const std::string& hexString)
        {
            if (hexString.size() < 32)
                throw std::invalid_argument("FGuid hex string must be at least 32 characters");
            A = ParseHex8(hexString, 0);
            B = ParseHex8(hexString, 8);
            C = ParseHex8(hexString, 16);
            D = ParseHex8(hexString, 24);
        }

        bool IsValid() const { return (A | B | C | D) != 0; }

        // The raw 16 bytes (little-endian, as stored). Valid until this FGuid is modified/destroyed.
        std::array<uint8_t, Size> AsByteSpan() const
        {
            std::array<uint8_t, Size> bytes{};
            WriteLE(bytes.data() + 0, A);
            WriteLE(bytes.data() + 4, B);
            WriteLE(bytes.data() + 8, C);
            WriteLE(bytes.data() + 12, D);
            return bytes;
        }

        std::string ToString(EGuidFormats guidFormat) const
        {
            char buf[128];
            switch (guidFormat)
            {
                case EGuidFormats::DigitsWithHyphens:
                    std::snprintf(buf, sizeof(buf), "%08X-%04X-%04X-%04X-%04X%08X",
                                  A, B >> 16, B & 0xFFFF, C >> 16, C & 0xFFFF, D);
                    return buf;
                case EGuidFormats::DigitsWithHyphensInBraces:
                    std::snprintf(buf, sizeof(buf), "{%08X-%04X-%04X-%04X-%04X%08X}",
                                  A, B >> 16, B & 0xFFFF, C >> 16, C & 0xFFFF, D);
                    return buf;
                case EGuidFormats::DigitsWithHyphensInParentheses:
                    std::snprintf(buf, sizeof(buf), "(%08X-%04X-%04X-%04X-%04X%08X)",
                                  A, B >> 16, B & 0xFFFF, C >> 16, C & 0xFFFF, D);
                    return buf;
                case EGuidFormats::HexValuesInBraces:
                    std::snprintf(buf, sizeof(buf),
                                  "{0x%08X,0x%04X,0x%04X,{0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X}}",
                                  A, B >> 16, B & 0xFFFF,
                                  C >> 24, (C >> 16) & 0xFF, (C >> 8) & 0xFF, C & 0xFF,
                                  D >> 24, (D >> 16) & 0xFF, (D >> 8) & 0xFF, D & 0xFF);
                    return buf;
                case EGuidFormats::UniqueObjectGuid:
                    std::snprintf(buf, sizeof(buf), "%08X-%08X-%08X-%08X", A, B, C, D);
                    return buf;
                case EGuidFormats::Short:
                {
                    auto data = AsByteSpan();
                    std::string result = Base64UrlSafe(data.data(), data.size());
                    if (result.size() == 24) // Remove trailing '=' base64 padding
                        result = result.substr(0, result.size() - 2);
                    return result;
                }
                case EGuidFormats::Base36Encoded:
                {
                    static const char alphabet[36] = {
                        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
                        'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
                        'W', 'X', 'Y', 'Z'
                    };
                    Math::FUInt128 zero(0);
                    Math::FUInt128 value(A, B, C, D);
                    std::string s;
                    while (value.IsGreater(zero))
                    {
                        uint32_t remainder = 0;
                        value = value.Divide(36, remainder);
                        s.insert(s.begin(), alphabet[remainder]);
                    }
                    for (int i = static_cast<int>(s.size()); i < 25; i++)
                        s.insert(s.begin(), '0');
                    s.insert(s.begin(), '0'); // mirrors C#'s builder.Insert(0, 0)
                    return s;
                }
                default:
                    std::snprintf(buf, sizeof(buf), "%08X%08X%08X%08X", A, B, C, D);
                    return buf;
            }
        }

        std::string ToString() const { return ToString(EGuidFormats::Digits); }

        bool Equals(const FGuid& other) const
        {
            return A == other.A && B == other.B && C == other.C && D == other.D;
        }

        bool operator==(const FGuid& other) const { return Equals(other); }
        bool operator!=(const FGuid& other) const { return !Equals(other); }

    private:
        static void WriteLE(uint8_t* p, uint32_t v)
        {
            p[0] = static_cast<uint8_t>(v);
            p[1] = static_cast<uint8_t>(v >> 8);
            p[2] = static_cast<uint8_t>(v >> 16);
            p[3] = static_cast<uint8_t>(v >> 24);
        }

        static uint32_t ParseHex8(const std::string& s, size_t offset)
        {
            uint32_t value = 0;
            for (size_t i = 0; i < 8; i++)
            {
                char ch = s[offset + i];
                uint32_t digit;
                if (ch >= '0' && ch <= '9') digit = static_cast<uint32_t>(ch - '0');
                else if (ch >= 'a' && ch <= 'f') digit = static_cast<uint32_t>(ch - 'a' + 10);
                else if (ch >= 'A' && ch <= 'F') digit = static_cast<uint32_t>(ch - 'A' + 10);
                else throw std::invalid_argument("Invalid hex digit in FGuid string");
                value = (value << 4) | digit;
            }
            return value;
        }

        static std::string Base64UrlSafe(const uint8_t* data, size_t len)
        {
            static const char* tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            std::string out;
            out.reserve(((len + 2) / 3) * 4);
            size_t i = 0;
            for (; i + 3 <= len; i += 3)
            {
                uint32_t n = (static_cast<uint32_t>(data[i]) << 16) |
                             (static_cast<uint32_t>(data[i + 1]) << 8) |
                             static_cast<uint32_t>(data[i + 2]);
                out.push_back(tbl[(n >> 18) & 0x3F]);
                out.push_back(tbl[(n >> 12) & 0x3F]);
                out.push_back(tbl[(n >> 6) & 0x3F]);
                out.push_back(tbl[n & 0x3F]);
            }
            const size_t rem = len - i;
            if (rem == 1)
            {
                uint32_t n = static_cast<uint32_t>(data[i]) << 16;
                out.push_back(tbl[(n >> 18) & 0x3F]);
                out.push_back(tbl[(n >> 12) & 0x3F]);
                out.push_back('=');
                out.push_back('=');
            }
            else if (rem == 2)
            {
                uint32_t n = (static_cast<uint32_t>(data[i]) << 16) |
                             (static_cast<uint32_t>(data[i + 1]) << 8);
                out.push_back(tbl[(n >> 18) & 0x3F]);
                out.push_back(tbl[(n >> 12) & 0x3F]);
                out.push_back(tbl[(n >> 6) & 0x3F]);
                out.push_back('=');
            }
            // URL-safe substitutions matching the C# .Replace('+','-').Replace('/','_').
            for (char& ch : out)
            {
                if (ch == '+') ch = '-';
                else if (ch == '/') ch = '_';
            }
            return out;
        }
    };
}
