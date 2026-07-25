// Ported from CUE4Parse/UE4/Wwise/Objects/HIRC/BaseHierarchyMusic.cs
#pragma once

#include <cstdint>
#include <vector>

#include "../../WwiseArchive.h"
#include "../../Enums/Flags/EMusicFlags.h"
#include "../AkChildren.h"
#include "AbstractHierarchy.h"
#include "BaseHierarchy.h"

namespace CUE4Parse::UE4::Wwise::Objects::HIRC
{
    using CUE4Parse::UE4::Wwise::Enums::Flags::EMusicFlags;

    class BaseHierarchyMusic : public AbstractHierarchy
    {
    public:
        BaseHierarchy ContainerHierarchy;
        EMusicFlags Flags = EMusicFlags::None;
        std::vector<uint32_t> ChildIds;

        // Note the flags come *before* the nested node params, and only exist past version 89.
        explicit BaseHierarchyMusic(FWwiseArchive& Ar)
        {
            Id = Ar.Read<uint32_t>();
            Flags = Ar.Version > 89 ? Ar.Read<EMusicFlags>() : EMusicFlags::None;
            ContainerHierarchy = BaseHierarchy(Ar);
            ChildIds = AkChildren(Ar).ChildIds;
        }
    };
}
