// Ported from CUE4Parse/UE4/Wwise/Objects/HIRC/Containers/HierarchyActorMixer.cs
#pragma once

#include <cstdint>
#include <vector>

#include "../../../WwiseArchive.h"
#include "../../AkChildren.h"
#include "../AbstractHierarchy.h"
#include "../BaseHierarchy.h"

namespace CUE4Parse::UE4::Wwise::Objects::HIRC::Containers
{
    // CAkActorMixer
    class HierarchyActorMixer : public AbstractHierarchy
    {
    public:
        BaseHierarchy BaseParams;
        std::vector<uint32_t> ChildIds;

        // CAkActorMixer::SetInitialValues
        explicit HierarchyActorMixer(FWwiseArchive& Ar)
        {
            Id = Ar.Read<uint32_t>();
            BaseParams = BaseHierarchy(Ar);
            ChildIds = AkChildren(Ar).ChildIds;
        }
    };
}
