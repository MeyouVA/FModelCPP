// Ported from CUE4Parse/UE4/IO/Objects/FIoContainerId.cs
#pragma once

#include <cstdint>
#include <string>

namespace CUE4Parse::UE4::IO::Objects
{
    struct FIoContainerId
    {
        uint64_t Id = 0;

        std::string ToString() const { return std::to_string(Id); }
    };
    static_assert(sizeof(FIoContainerId) == 8);
}
