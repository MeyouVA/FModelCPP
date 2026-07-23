// Ported from CUE4Parse/UE4/IO/Objects/FIoDirectoryIndexEntry.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::IO::Objects
{
    struct FIoDirectoryIndexEntry
    {
        uint32_t Name = 0;
        uint32_t FirstChildEntry = 0;
        uint32_t NextSiblingEntry = 0;
        uint32_t FirstFileEntry = 0;
    };
    static_assert(sizeof(FIoDirectoryIndexEntry) == 16);
}
