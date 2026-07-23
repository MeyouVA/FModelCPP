// Ported from CUE4Parse/UE4/IO/Objects/FIoOffsetAndLength.cs
// Ten bytes: a 5-byte big-endian offset followed by a 5-byte big-endian length.
#pragma once

#include <cstdint>
#include <cstdio>
#include <string>

namespace CUE4Parse::UE4::IO::Objects
{
    struct FIoOffsetAndLength
    {
        uint8_t OffsetAndLength[10] = {};

        uint64_t Offset() const
        {
            return static_cast<uint64_t>(OffsetAndLength[4])
                 | (static_cast<uint64_t>(OffsetAndLength[3]) << 8)
                 | (static_cast<uint64_t>(OffsetAndLength[2]) << 16)
                 | (static_cast<uint64_t>(OffsetAndLength[1]) << 24)
                 | (static_cast<uint64_t>(OffsetAndLength[0]) << 32);
        }

        uint64_t Length() const
        {
            return static_cast<uint64_t>(OffsetAndLength[9])
                 | (static_cast<uint64_t>(OffsetAndLength[8]) << 8)
                 | (static_cast<uint64_t>(OffsetAndLength[7]) << 16)
                 | (static_cast<uint64_t>(OffsetAndLength[6]) << 24)
                 | (static_cast<uint64_t>(OffsetAndLength[5]) << 32);
        }

        std::string ToString() const
        {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "Offset %llu | Length %llu",
                          static_cast<unsigned long long>(Offset()), static_cast<unsigned long long>(Length()));
            return buf;
        }
    };
    static_assert(sizeof(FIoOffsetAndLength) == 10, "FIoOffsetAndLength must match the 10-byte on-disk layout");
}
