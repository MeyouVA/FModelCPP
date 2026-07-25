// Ported from CUE4Parse/UE4/Wwise/Objects/HIRC/Containers/HierarchyLayerContainer.cs
#pragma once

#include <cstdint>
#include <vector>

#include "../../../WwiseArchive.h"
#include "../../AkChildren.h"
#include "../../CAkLayer.h"
#include "../AbstractHierarchy.h"
#include "../BaseHierarchy.h"

namespace CUE4Parse::UE4::Wwise::Objects::HIRC::Containers
{
    class HierarchyLayerContainer : public AbstractHierarchy
    {
    public:
        BaseHierarchy BaseParams;
        std::vector<uint32_t> ChildIds;
        std::vector<CAkLayer> Layers;
        bool IsContinuousValidation = false;

        // CAkBankMgr::StdBankRead<CAkLayerCntr>
        // CAkLayerCntr::SetInitialValues
        explicit HierarchyLayerContainer(FWwiseArchive& Ar)
        {
            Id = Ar.Read<uint32_t>();
            BaseParams = BaseHierarchy(Ar);
            ChildIds = AkChildren(Ar).ChildIds;
            const int layerCount = static_cast<int>(Ar.Read<uint32_t>());
            Layers = Ar.ReadArrayWith(layerCount, [&Ar] { return CAkLayer(Ar); });

            if (Ar.Version > 118)
            {
                IsContinuousValidation = Ar.ReadBool();
            }
        }
    };
}
