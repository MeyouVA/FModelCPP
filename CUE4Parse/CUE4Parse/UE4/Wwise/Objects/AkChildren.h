// Ported from CUE4Parse/UE4/Wwise/Objects/AkChildren.cs
#pragma once

#include <cstdint>
#include <vector>

#include "../WwiseArchive.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    struct AkChildren
    {
        std::vector<uint32_t> ChildIds;

        AkChildren() = default;

        explicit AkChildren(FWwiseArchive& Ar)
        {
            ChildIds = Ar.ReadArray<uint32_t>(static_cast<int>(Ar.Read<uint32_t>()));
        }
    };
}
