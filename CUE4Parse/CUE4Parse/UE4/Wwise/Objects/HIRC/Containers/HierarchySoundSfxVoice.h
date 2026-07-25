// Ported from CUE4Parse/UE4/Wwise/Objects/HIRC/Containers/HierarchySoundSfxVoice.cs
#pragma once

#include <cstdint>

#include "../../../WwiseArchive.h"
#include "../../AkBankSourceData.h"
#include "../AbstractHierarchy.h"
#include "../BaseHierarchy.h"

namespace CUE4Parse::UE4::Wwise::Objects::HIRC::Containers
{
    class HierarchySoundSfxVoice : public AbstractHierarchy
    {
    public:
        AkBankSourceData Source;
        BaseHierarchy BaseParams;

        // CAkSound::SetInitialValues
        explicit HierarchySoundSfxVoice(FWwiseArchive& Ar)
        {
            Id = Ar.Read<uint32_t>();
            Source = AkBankSourceData(Ar);
            BaseParams = BaseHierarchy(Ar);
        }
    };
}
