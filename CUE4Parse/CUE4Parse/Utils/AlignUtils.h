// Ported from CUE4Parse/Utils/AlignUtils.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::Utils
{
    // Rounds ptr up to the next multiple of alignment (alignment must be a power of two).
    inline int64_t Align(int64_t ptr, int32_t alignment)
    {
        return (ptr + alignment - 1) & ~static_cast<int64_t>(alignment - 1);
    }

    inline int32_t Align(int32_t ptr, int32_t alignment)
    {
        return (ptr + alignment - 1) & ~(alignment - 1);
    }

    inline int64_t Align(uint32_t ptr, int32_t alignment)
    {
        return (static_cast<int64_t>(ptr) + alignment - 1) & ~static_cast<int64_t>(alignment - 1);
    }
}
