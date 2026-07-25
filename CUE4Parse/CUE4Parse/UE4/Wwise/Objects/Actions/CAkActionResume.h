// Ported from CUE4Parse/UE4/Wwise/Objects/Actions/CAkActionResume.cs
#pragma once

#include <cstdint>

#include "../../WwiseArchive.h"
#include "../../Enums/Flags/EResumeOptionsFlags.h"
#include "CAkActionExcept.h"
#include "CAkActionParams.h"

namespace CUE4Parse::UE4::Wwise::Objects::Actions
{
    using CUE4Parse::UE4::Wwise::Enums::Flags::EResumeOptionsFlags;

    class CAkActionResume
    {
    public:
        CAkActionParams ActionParams;
        EResumeOptionsFlags ResumeOptions = static_cast<EResumeOptionsFlags>(0);
        CAkActionExcept ExceptParams;

        CAkActionResume() = default;

        // CAkActionResume::SetActionActiveParams
        explicit CAkActionResume(FWwiseArchive& Ar)
        {
            ActionParams = CAkActionParams(Ar);

            if (Ar.Version <= 56)
                Ar.Read<uint32_t>(); // IsMaster
            else if (Ar.Version <= 62)
                Ar.Read<uint8_t>();  // IsMaster
            else
                ResumeOptions = Ar.Read<EResumeOptionsFlags>();

            ExceptParams = CAkActionExcept(Ar);
        }
    };
}
