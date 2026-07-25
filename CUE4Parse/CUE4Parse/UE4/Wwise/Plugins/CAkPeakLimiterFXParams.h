// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkPeakLimiterFXParams.cs
#pragma once

#include <cstdint>

#include "../WwiseArchive.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    struct AkPeakLimiterRTPCParams
    {
        float fThreshold = 0;
        float fRatio = 0;
        float fRelease = 0;
        float fOutputLevel = 0;
    };

    struct AkPeakLimiterNonRTPCParams
    {
        float fLookAhead = 0;
        bool bProcessLFE = false;
        bool bChannelLink = false;
    };

    struct AkPeakLimiterFXParams
    {
        AkPeakLimiterRTPCParams RTPC;
        AkPeakLimiterNonRTPCParams NonRTPC;

        AkPeakLimiterFXParams() = default;

        explicit AkPeakLimiterFXParams(FWwiseArchive& Ar)
        {
            RTPC.fThreshold = Ar.Read<float>();
            RTPC.fRatio = Ar.Read<float>();
            NonRTPC.fLookAhead = Ar.Read<float>(); // interleaved -- see AkDelayFXParams
            RTPC.fRelease = Ar.Read<float>();
            RTPC.fOutputLevel = DbToLinear(Ar.Read<float>());
            NonRTPC.bProcessLFE = Ar.Read<uint8_t>() != 0;
            NonRTPC.bChannelLink = Ar.Read<uint8_t>() != 0;
        }
    };

    class CAkPeakLimiterFXParams : public IAkPluginParam
    {
    public:
        AkPeakLimiterFXParams Params;

        explicit CAkPeakLimiterFXParams(FWwiseArchive& Ar) : Params(Ar) {}
    };
}
