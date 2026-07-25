// Ported from CUE4Parse/UE4/Wwise/Objects/AkStateGroupInfo.cs
#pragma once

#include <cstdint>
#include <vector>

#include "../WwiseArchive.h"
#include "AkStateTransition.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    struct AkStateGroupInfo
    {
        uint32_t StateGroupId = 0;
        uint32_t DefaultTransitionTime = 0;
        std::vector<AkStateTransition> StateTransitions;

        AkStateGroupInfo() = default;

        explicit AkStateGroupInfo(FWwiseArchive& Ar)
        {
            StateGroupId = Ar.Read<uint32_t>();
            DefaultTransitionTime = Ar.Read<uint32_t>();
            const int count = static_cast<int>(Ar.Read<uint32_t>());
            StateTransitions = Ar.ReadArrayWith(count, [&Ar] { return AkStateTransition(Ar); });
        }
    };
}
