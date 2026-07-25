// Ported from CUE4Parse/UE4/Wwise/Objects/HIRC/Containers/HierarchySidechainMix.cs
#pragma once

#include <cstdint>

#include "../../../WwiseArchive.h"
#include "../../AkChannelConfig.h"
#include "../AbstractHierarchy.h"

namespace CUE4Parse::UE4::Wwise::Objects::HIRC::Containers
{
    class HierarchySidechainMix : public AbstractHierarchy
    {
    public:
        AkChannelConfig ChannelConfig;

        // CAkSidechainMixIndexable::SetInitialValues
        explicit HierarchySidechainMix(FWwiseArchive& Ar)
        {
            Id = Ar.Read<uint32_t>();
            ChannelConfig = AkChannelConfig(Ar);
        }
    };
}
