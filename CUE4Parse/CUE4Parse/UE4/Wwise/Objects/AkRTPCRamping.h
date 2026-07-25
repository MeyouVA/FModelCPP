// Ported from CUE4Parse/UE4/Wwise/Objects/AkRTPCRamping.cs
#pragma once

#include <cstdint>

#include "../WwiseArchive.h"
#include "../Enums/EAkBuiltInParam.h"
#include "../Enums/EAkTransitionRampingType.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    using CUE4Parse::UE4::Wwise::Enums::EAkBuiltInParam;
    using CUE4Parse::UE4::Wwise::Enums::EAkTransitionRampingType;

    struct AkRTPCRamping
    {
        uint32_t RtpcId = 0;
        float Value = 0;
        EAkTransitionRampingType RampType = static_cast<EAkTransitionRampingType>(0);
        float RampUp = 0;
        float RampDown = 0;
        EAkBuiltInParam BindToBuiltInParam = static_cast<EAkBuiltInParam>(0);

        AkRTPCRamping() = default;

        explicit AkRTPCRamping(FWwiseArchive& Ar)
        {
            RtpcId = Ar.Read<uint32_t>();
            Value = Ar.Read<float>();

            if (Ar.Version > 89)
            {
                RampType = Ar.Read<EAkTransitionRampingType>();
                RampUp = Ar.Read<float>();
                RampDown = Ar.Read<float>();
                // CAkGameSyncMgr::BindGameSyncToBuiltIn
                BindToBuiltInParam = Ar.Read<EAkBuiltInParam>();
            }
        }
    };
}
