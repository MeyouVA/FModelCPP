// Ported from CUE4Parse/UE4/Wwise/Plugins/CAk3DAudioBedMixerFXParams.cs
#pragma once

#include <cstdint>

#include "../WwiseArchive.h"
#include "../Enums/EAkChannelConfig.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    using CUE4Parse::UE4::Wwise::Enums::EAkChannelConfig;

    struct Ak3DAudioBedMixerRTPCParams
    {
        EAkChannelConfig MainMixConfiguration = static_cast<EAkChannelConfig>(0);
        uint16_t PassthroughMixPolicy = 0;
        uint16_t SystemAudioObjectsPolicy = 0;
        uint16_t SystemAudioObjectLimit = 0;

        Ak3DAudioBedMixerRTPCParams() = default;

        explicit Ak3DAudioBedMixerRTPCParams(FWwiseArchive& Ar)
        {
            MainMixConfiguration = Ar.Read<EAkChannelConfig>();
            PassthroughMixPolicy = Ar.Read<uint16_t>();
            SystemAudioObjectsPolicy = Ar.Read<uint16_t>();
            SystemAudioObjectLimit = Ar.Read<uint16_t>();
        }
    };

    class CAk3DAudioBedMixerFXParams : public IAkPluginParam
    {
    public:
        Ak3DAudioBedMixerRTPCParams Params;

        explicit CAk3DAudioBedMixerFXParams(FWwiseArchive& Ar) : Params(Ar) {}
    };
}
