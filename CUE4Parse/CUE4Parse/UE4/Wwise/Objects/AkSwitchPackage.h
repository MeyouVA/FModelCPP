// Ported from CUE4Parse/UE4/Wwise/Objects/AkSwitchPackage.cs
#pragma once

#include <cstdint>
#include <vector>

#include "../WwiseArchive.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    struct AkSwitchPackage
    {
        uint32_t SwitchId = 0;
        std::vector<uint32_t> NodeIds;

        AkSwitchPackage() = default;

        explicit AkSwitchPackage(FWwiseArchive& Ar)
        {
            SwitchId = Ar.Read<uint32_t>();
            NodeIds = Ar.ReadArray<uint32_t>(static_cast<int>(Ar.Read<uint32_t>()));
        }
    };
}
