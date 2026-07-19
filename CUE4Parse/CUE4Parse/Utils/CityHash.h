// Ported from CUE4Parse/Utils/CityHash.cs  (https://github.com/google/cityhash)
#pragma once

#include <cstdint>
#include <vector>

namespace CUE4Parse::Utils
{
    class CityHash
    {
    public:
        static uint64_t CityHash64(const uint8_t* buffer, uint32_t len);
        static uint64_t CityHash64(const std::vector<uint8_t>& buffer);
        static uint64_t CityHash64WithSeed(const std::vector<uint8_t>& buffer, uint64_t seed);
        static uint64_t CityHash64WithSeeds(const std::vector<uint8_t>& buffer, uint64_t seed0, uint64_t seed1);
    };
}
