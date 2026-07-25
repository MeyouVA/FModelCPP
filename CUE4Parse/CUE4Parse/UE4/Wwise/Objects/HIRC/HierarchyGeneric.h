// Ported from CUE4Parse/UE4/Wwise/Objects/HIRC/HierarchyGeneric.cs
#pragma once

#include "../../WwiseArchive.h"
#include "AbstractHierarchy.h"

namespace CUE4Parse::UE4::Wwise::Objects::HIRC
{
    // The fallback for an unknown or unparseable hierarchy: read the id and nothing else. Hierarchy
    // rewinds and re-reads through this when a typed parse throws.
    class HierarchyGeneric : public AbstractHierarchy
    {
    public:
        explicit HierarchyGeneric(FWwiseArchive& Ar) { Id = Ar.Read<uint32_t>(); }
    };
}
