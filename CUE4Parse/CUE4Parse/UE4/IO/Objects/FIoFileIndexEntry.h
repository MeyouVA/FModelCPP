// Ported from CUE4Parse/UE4/IO/Objects/FIoFileIndexEntry.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::IO::Objects
{
    struct FIoFileIndexEntry
    {
        uint32_t Name = 0;
        uint32_t NextFileEntry = 0;
        uint32_t UserData = 0;
    };
    static_assert(sizeof(FIoFileIndexEntry) == 12);
}
