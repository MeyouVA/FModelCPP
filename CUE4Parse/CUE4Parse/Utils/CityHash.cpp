// Ported from CUE4Parse/Utils/CityHash.cs
#include "CityHash.h"

#include <cstring>
#include <stdexcept>
#include <utility>

namespace CUE4Parse::Utils
{
    namespace
    {
        constexpr uint64_t K0 = 0xc3a5c85c97cb3127ULL;
        constexpr uint64_t K1 = 0xb492b66fbe98f273ULL;
        constexpr uint64_t K2 = 0x9ae16a3b2f90404fULL;

        using U128 = std::pair<uint64_t, uint64_t>; // (Item1, Item2)

        // Unaligned little-endian loads (C# reads native-endian via *(T*)p on x86/x64 which is LE).
        inline uint32_t Fetch32(const uint8_t* p) { uint32_t v; std::memcpy(&v, p, sizeof(v)); return v; }
        inline uint64_t Fetch64(const uint8_t* p) { uint64_t v; std::memcpy(&v, p, sizeof(v)); return v; }

        inline uint64_t Rotate(uint64_t val, int shift) { return shift == 0 ? val : (val >> shift) | (val << (64 - shift)); }
        inline uint64_t ShiftMix(uint64_t val) { return val ^ (val >> 47); }

        inline uint64_t Bswap_64(uint64_t value)
        {
            value = ((value << 8) & 0xFF00FF00FF00FF00ULL) | ((value >> 8) & 0x00FF00FF00FF00FFULL);
            value = ((value << 16) & 0xFFFF0000FFFF0000ULL) | ((value >> 16) & 0x0000FFFF0000FFFFULL);
            return (value << 32) | (value >> 32);
        }

        inline uint64_t Hash128To64(const U128& x)
        {
            constexpr uint64_t kMul = 0x9ddfea08eb382d69ULL;
            uint64_t a = (x.first ^ x.second) * kMul;
            a ^= a >> 47;
            uint64_t b = (x.second ^ a) * kMul;
            b ^= b >> 47;
            b *= kMul;
            return b;
        }

        inline uint64_t HashLen16(uint64_t u, uint64_t v) { return Hash128To64({u, v}); }

        inline uint64_t HashLen16(uint64_t u, uint64_t v, uint64_t mul)
        {
            uint64_t a = (u ^ v) * mul;
            a ^= a >> 47;
            uint64_t b = (v ^ a) * mul;
            b ^= b >> 47;
            b *= mul;
            return b;
        }

        uint64_t HashLen0to16(const uint8_t* s, uint32_t len)
        {
            if (len >= 8)
            {
                uint64_t mul = K2 + len * 2;
                uint64_t a = Fetch64(s) + K2;
                uint64_t b = Fetch64(s + len - 8);
                uint64_t c = Rotate(b, 37) * mul + a;
                uint64_t d = (Rotate(a, 25) + b) * mul;
                return HashLen16(c, d, mul);
            }
            if (len >= 4)
            {
                uint64_t mul = K2 + len * 2;
                uint64_t a = Fetch32(s);
                return HashLen16(len + (a << 3), Fetch32(s + len - 4), mul);
            }
            if (len > 0)
            {
                uint8_t a = s[0];
                uint8_t b = s[len >> 1];
                uint8_t c = s[len - 1];
                uint32_t y = static_cast<uint32_t>(a) + (static_cast<uint32_t>(b) << 8);
                uint32_t z = len + (static_cast<uint32_t>(c) << 2);
                return ShiftMix(y * K2 ^ z * K0) * K2;
            }
            return K2;
        }

        uint64_t HashLen17to32(const uint8_t* s, uint32_t len)
        {
            uint64_t mul = K2 + len * 2;
            uint64_t a = Fetch64(s) * K1;
            uint64_t b = Fetch64(s + 8);
            uint64_t c = Fetch64(s + len - 8) * mul;
            uint64_t d = Fetch64(s + len - 16) * K2;
            return HashLen16(Rotate(a + b, 43) + Rotate(c, 30) + d, a + Rotate(b + K2, 18) + c, mul);
        }

        uint64_t HashLen33to64(const uint8_t* s, uint32_t len)
        {
            uint64_t mul = K2 + len * 2;
            uint64_t a = Fetch64(s) * K2;
            uint64_t b = Fetch64(s + 8);
            uint64_t c = Fetch64(s + len - 24);
            uint64_t d = Fetch64(s + len - 32);
            uint64_t e = Fetch64(s + 16) * K2;
            uint64_t f = Fetch64(s + 24) * 9;
            uint64_t g = Fetch64(s + len - 8);
            uint64_t h = Fetch64(s + len - 16) * mul;
            uint64_t u = Rotate(a + g, 43) + (Rotate(b, 30) + c) * 9;
            uint64_t v = ((a + g) ^ d) + f + 1;
            uint64_t w = Bswap_64((u + v) * mul) + h;
            uint64_t x = Rotate(e + f, 42) + c;
            uint64_t y = (Bswap_64((v + w) * mul) + g) * mul;
            uint64_t z = e + f + c;
            a = Bswap_64((x + z) * mul + y) + b;
            b = ShiftMix((z + a) * mul + d + h) * mul;
            return b + x;
        }

        U128 WeakHashLen32WithSeeds(uint64_t w, uint64_t x, uint64_t y, uint64_t z, uint64_t a, uint64_t b)
        {
            a += w;
            b = Rotate(b + a + z, 21);
            uint64_t c = a;
            a += x;
            a += y;
            b += Rotate(a, 44);
            return {a + z, b + c};
        }

        U128 WeakHashLen32WithSeeds(const uint8_t* s, uint64_t a, uint64_t b)
        {
            return WeakHashLen32WithSeeds(Fetch64(s), Fetch64(s + 8), Fetch64(s + 16), Fetch64(s + 24), a, b);
        }
    }

    uint64_t CityHash::CityHash64(const uint8_t* buffer, uint32_t len)
    {
        const uint8_t* s = buffer;

        if (len <= 32)
            return len <= 16 ? HashLen0to16(s, len) : HashLen17to32(s, len);
        if (len <= 64)
            return HashLen33to64(s, len);

        uint64_t x = Fetch64(s + len - 40);
        uint64_t y = Fetch64(s + len - 16) + Fetch64(s + len - 56);
        uint64_t z = HashLen16(Fetch64(s + len - 48) + len, Fetch64(s + len - 24));
        U128 v = WeakHashLen32WithSeeds(s + len - 64, len, z);
        U128 w = WeakHashLen32WithSeeds(s + len - 32, y + K1, x);
        x = x * K1 + Fetch64(s);
        len = (len - 1) & ~static_cast<uint32_t>(63);

        do
        {
            x = Rotate(x + y + v.first + Fetch64(s + 8), 37) * K1;
            y = Rotate(y + v.second + Fetch64(s + 48), 42) * K1;
            x ^= w.second;
            y += v.first + Fetch64(s + 40);
            z = Rotate(z + w.first, 33) * K1;
            v = WeakHashLen32WithSeeds(s, v.second * K1, x + w.first);
            w = WeakHashLen32WithSeeds(s + 32, z + w.second, y + Fetch64(s + 16));
            std::swap(z, x);
            s += 64;
            len -= 64;
        } while (len != 0);

        return HashLen16(HashLen16(v.first, w.first) + ShiftMix(y) * K1 + z, HashLen16(v.second, w.second) + x);
    }

    uint64_t CityHash::CityHash64(const std::vector<uint8_t>& buffer)
    {
        if (buffer.empty())
            throw std::invalid_argument("buffer");
        return CityHash64(buffer.data(), static_cast<uint32_t>(buffer.size()));
    }

    uint64_t CityHash::CityHash64WithSeed(const std::vector<uint8_t>& buffer, uint64_t seed)
    {
        return CityHash64WithSeeds(buffer, K2, seed);
    }

    uint64_t CityHash::CityHash64WithSeeds(const std::vector<uint8_t>& buffer, uint64_t seed0, uint64_t seed1)
    {
        return HashLen16(CityHash64(buffer) - seed0, seed1);
    }
}
