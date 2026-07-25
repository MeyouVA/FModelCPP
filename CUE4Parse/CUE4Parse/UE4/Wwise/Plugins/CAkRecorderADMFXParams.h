// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkRecorderADMFXParams.cs
#pragma once

#include <cstdint>
#include <string>

#include "../WwiseArchive.h"
#include "../Enums/EAkChannelConfig.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    using CUE4Parse::UE4::Wwise::Enums::EAkChannelConfig;

    struct AkRecorderADMFXParams
    {
        int16_t Profile = 0;
        int16_t ChannelCount = 0;
        EAkChannelConfig MainMixChannelConfig = static_cast<EAkChannelConfig>(0);
        bool Passthrough = false;
        bool PreserveExtraBeds = false;
        bool ApplyDownstreamVolume = false;
        bool Hold = false;
        std::string GameFilename;

        AkRecorderADMFXParams() = default;

        explicit AkRecorderADMFXParams(FWwiseArchive& Ar)
        {
            Profile = Ar.Read<int16_t>();
            ChannelCount = Ar.Read<int16_t>();
            MainMixChannelConfig = Ar.Read<EAkChannelConfig>();
            Passthrough = Ar.Read<uint8_t>() != 0;
            PreserveExtraBeds = Ar.Read<uint8_t>() != 0;
            ApplyDownstreamVolume = Ar.Read<uint8_t>() != 0;
            Hold = Ar.Read<uint8_t>() != 0;
            GameFilename = Ar.ReadStzString();
        }
    };

    class CAkRecorderADMFXParams : public IAkPluginParam
    {
    public:
        AkRecorderADMFXParams Params;

        explicit CAkRecorderADMFXParams(FWwiseArchive& Ar) : Params(Ar) {}
    };
}
