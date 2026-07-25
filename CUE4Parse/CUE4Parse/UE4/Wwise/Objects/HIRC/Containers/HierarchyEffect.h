// Ported from CUE4Parse/UE4/Wwise/Objects/HIRC/Containers/HierarchyEffect.cs
#pragma once

#include <cstdint>

#include "../../../WwiseArchive.h"
#include "../AbstractHierarchy.h"

namespace CUE4Parse::UE4::Wwise::Objects::HIRC::Containers
{
    // Reads only the id -- identical in effect to HierarchyGeneric, but a distinct type in C#.
    class HierarchyEffect : public AbstractHierarchy
    {
    public:
        explicit HierarchyEffect(FWwiseArchive& Ar) { Id = Ar.Read<uint32_t>(); }
    };
}
