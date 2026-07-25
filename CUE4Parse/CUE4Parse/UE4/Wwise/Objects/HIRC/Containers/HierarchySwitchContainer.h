// Ported from CUE4Parse/UE4/Wwise/Objects/HIRC/Containers/HierarchySwitchContainer.cs
#pragma once

#include <cstdint>
#include <vector>

#include "../../../WwiseArchive.h"
#include "../../../Enums/EAkGroupType.h"
#include "../../AkChildren.h"
#include "../../AkSwitchPackage.h"
#include "../../AkSwitchParams.h"
#include "../AbstractHierarchy.h"
#include "../BaseHierarchy.h"

namespace CUE4Parse::UE4::Wwise::Objects::HIRC::Containers
{
    using CUE4Parse::UE4::Wwise::Enums::EAkGroupType;

    // CAkSwitchCntr
    class HierarchySwitchContainer : public AbstractHierarchy
    {
    public:
        BaseHierarchy BaseParams;
        EAkGroupType GroupType = static_cast<EAkGroupType>(0);
        uint32_t GroupId = 0;
        uint32_t DefaultSwitch = 0;
        bool IsContinuousValidation = false;
        std::vector<uint32_t> ChildIds;
        std::vector<AkSwitchPackage> SwitchPackages;
        std::vector<AkSwitchParams> SwitchParams;

        // CAkSwitchCntr::SetInitialValues
        explicit HierarchySwitchContainer(FWwiseArchive& Ar)
        {
            Id = Ar.Read<uint32_t>();
            BaseParams = BaseHierarchy(Ar);
            // The group type narrowed from uint to byte at 90.
            if (Ar.Version <= 89)
                GroupType = static_cast<EAkGroupType>(Ar.Read<uint32_t>());
            else
                GroupType = Ar.Read<EAkGroupType>();
            GroupId = Ar.Read<uint32_t>();
            DefaultSwitch = Ar.Read<uint32_t>();
            IsContinuousValidation = Ar.ReadBool();
            ChildIds = AkChildren(Ar).ChildIds;
            const int packageCount = static_cast<int>(Ar.Read<uint32_t>());
            SwitchPackages = Ar.ReadArrayWith(packageCount, [&Ar] { return AkSwitchPackage(Ar); });
            const int paramCount = static_cast<int>(Ar.Read<uint32_t>());
            SwitchParams = Ar.ReadArrayWith(paramCount, [&Ar] { return AkSwitchParams(Ar); });
        }
    };
}
