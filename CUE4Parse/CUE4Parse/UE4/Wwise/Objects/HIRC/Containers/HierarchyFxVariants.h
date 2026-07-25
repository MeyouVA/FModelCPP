// Ported from HierarchyFxCustom.cs, HierarchyFxShareSet.cs and HierarchyAudioDevice.cs -- the three
// BaseHierarchyFx subclasses. The first two add nothing; the audio device adds an effect-slot block.
#pragma once

#include "../../../WwiseArchive.h"
#include "../../AkFXParams.h"
#include "../BaseHierarchyFx.h"

namespace CUE4Parse::UE4::Wwise::Objects::HIRC::Containers
{
    // CAkBankMgr::StdBankRead<CAkFxCustom>
    class HierarchyFxCustom : public BaseHierarchyFx
    {
    public:
        explicit HierarchyFxCustom(FWwiseArchive& Ar) : BaseHierarchyFx(Ar) {}
    };

    // CAkBankMgr::StdBankRead<CAkFxShareSet>
    class HierarchyFxShareSet : public BaseHierarchyFx
    {
    public:
        explicit HierarchyFxShareSet(FWwiseArchive& Ar) : BaseHierarchyFx(Ar) {}
    };

    class HierarchyAudioDevice : public BaseHierarchyFx
    {
    public:
        AkFxParams FxParams;

        // CAkAudioDevice::SetInitialValues
        explicit HierarchyAudioDevice(FWwiseArchive& Ar) : BaseHierarchyFx(Ar)
        {
            if (Ar.Version > 136)
            {
                FxParams = AkFxParams(Ar); // AkOwnedEffectSlots::SetInitialValues
            }
        }
    };
}
