// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkFDNReverbFXParams.cs
#pragma once

#include <cstdint>
#include <vector>

#include "../WwiseArchive.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    enum class AkDelayLengthsMode : uint32_t
    {
        DEFAULT = 0x0,
        CUSTOM = 0x1
    };

    class AkFDNReverbFXParams
    {
    public:
        struct AkFDNReverbRTPCParams
        {
            float fReverbTime = 0;
            float fHFRatio = 0;
            float fDryLevel = 0;
            float fWetLevel = 0;
        };

        struct AkFDNReverbNonRTPCParams
        {
            int32_t uNumberOfDelays = 0;
            float fPreDelay = 0;
            bool uProcessLFE = false;
            AkDelayLengthsMode uDelayLengthsMode = static_cast<AkDelayLengthsMode>(0);
            std::vector<float> fDelayTime;
        };

        AkFDNReverbRTPCParams RTPC;
        AkFDNReverbNonRTPCParams NonRTPC;

        AkFDNReverbFXParams() = default;

        explicit AkFDNReverbFXParams(FWwiseArchive& Ar)
        {
            RTPC.fReverbTime = Ar.Read<float>();
            RTPC.fHFRatio = Ar.Read<float>();
            NonRTPC.uNumberOfDelays = Ar.Read<int32_t>();
            RTPC.fDryLevel = DbToLinear(Ar.Read<float>());
            RTPC.fWetLevel = DbToLinear(Ar.Read<float>());
            NonRTPC.fPreDelay = Ar.Read<float>();
            NonRTPC.uProcessLFE = Ar.Read<uint8_t>() != 0;
            NonRTPC.uDelayLengthsMode = Ar.Read<AkDelayLengthsMode>();
            // The per-delay times only exist in CUSTOM mode; the count was read well before the mode.
            NonRTPC.fDelayTime = NonRTPC.uDelayLengthsMode == AkDelayLengthsMode::CUSTOM
                                     ? Ar.ReadArray<float>(NonRTPC.uNumberOfDelays)
                                     : std::vector<float>{};
        }
    };

    class CAkFDNReverbFXParams : public IAkPluginParam
    {
    public:
        AkFDNReverbFXParams Params;

        explicit CAkFDNReverbFXParams(FWwiseArchive& Ar) : Params(Ar) {}
    };
}
