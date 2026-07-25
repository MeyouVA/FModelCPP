// Ported from CUE4Parse/UE4/FMod/Utils/JenkinsHash.cs
// FMOD::hashlittle2 (Jenkins lookup3). Used to look a sound key up in the SoundTable.
// https://android.googlesource.com/platform/external/jenkins-hash/+/75dbeade.../lookup3.c
#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace CUE4Parse::UE4::FMod::Utils
{
    class JenkinsHash
    {
    public:
        static uint64_t Hash64(const std::string& input, uint32_t pc = 0, uint32_t pb = 0)
        {
            std::vector<uint8_t> data(input.begin(), input.end());
            HashLittle2(data.data(), data.size(), pc, pb);
            return (static_cast<uint64_t>(pb) << 32) | pc;
        }

        static void HashLittle2(const uint8_t* key, size_t keyLength, uint32_t& pc, uint32_t& pb)
        {
            uint32_t length = static_cast<uint32_t>(keyLength);
            uint32_t a, b, c;

            a = b = c = 0xdeadbeef + length + pc;
            c += pb;

            int offset = 0;
            int len = static_cast<int>(length);

            while (len > 12)
            {
                a += ReadU32(key, offset);
                b += ReadU32(key, offset + 4);
                c += ReadU32(key, offset + 8);

                a -= c; a ^= Rot(c, 4);  c += b;
                b -= a; b ^= Rot(a, 6);  a += c;
                c -= b; c ^= Rot(b, 8);  b += a;
                a -= c; a ^= Rot(c, 16); c += b;
                b -= a; b ^= Rot(a, 19); a += c;
                c -= b; c ^= Rot(b, 4);  b += a;

                offset += 12;
                len -= 12;
            }

            switch (len)
            {
                case 12: c += static_cast<uint32_t>(key[offset + 11]) << 24; [[fallthrough]];
                case 11: c += static_cast<uint32_t>(key[offset + 10]) << 16; [[fallthrough]];
                case 10: c += static_cast<uint32_t>(key[offset + 9]) << 8;   [[fallthrough]];
                case 9:  c += key[offset + 8];                               [[fallthrough]];
                case 8:  b += static_cast<uint32_t>(key[offset + 7]) << 24;  [[fallthrough]];
                case 7:  b += static_cast<uint32_t>(key[offset + 6]) << 16;  [[fallthrough]];
                case 6:  b += static_cast<uint32_t>(key[offset + 5]) << 8;   [[fallthrough]];
                case 5:  b += key[offset + 4];                               [[fallthrough]];
                case 4:  a += static_cast<uint32_t>(key[offset + 3]) << 24;  [[fallthrough]];
                case 3:  a += static_cast<uint32_t>(key[offset + 2]) << 16;  [[fallthrough]];
                case 2:  a += static_cast<uint32_t>(key[offset + 1]) << 8;   [[fallthrough]];
                case 1:  a += key[offset]; break;
                case 0:  pc = c; pb = b; return;
            }

            c ^= b; c -= Rot(b, 14);
            a ^= c; a -= Rot(c, 11);
            b ^= a; b -= Rot(a, 25);
            c ^= b; c -= Rot(b, 16);
            a ^= c; a -= Rot(c, 4);
            b ^= a; b -= Rot(a, 14);
            c ^= b; c -= Rot(b, 24);

            pc = c;
            pb = b;
        }

    private:
        static uint32_t Rot(uint32_t x, int k) { return (x << k) | (x >> (32 - k)); }

        static uint32_t ReadU32(const uint8_t* p, int offset)
        {
            uint32_t v;
            std::memcpy(&v, p + offset, sizeof(v)); // little-endian host, matching BitConverter.ToUInt32
            return v;
        }
    };
}
