// Ported from CUE4Parse/UE4/VirtualFileCache/EVFCFileVersion.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::VirtualFileCache
{
    enum class EVFCFileVersion
    {
        Invalid,
        Initial,
        Count,
        Current = Count - 1,
    };
}
