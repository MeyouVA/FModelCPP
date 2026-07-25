// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkDelayFXParams.cs
#pragma once

#include <cstdint>

#include "../WwiseArchive.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    struct AkDelayRTPCParams
    {
        float fFeedback = 0;
        float fWetDryMix = 0;
        float fOutputLevel = 0;
        bool bFeedbackEnabled = false;
    };

    struct AkDelayNonRTPCParams
    {
        float fDelayTime = 0;
        bool bProcessLFE = false;
    };

    struct AkDelayFXParams
    {
        AkDelayRTPCParams RTPC;
        AkDelayNonRTPCParams NonRTPC;

        AkDelayFXParams() = default;

        // The wire order interleaves the two groups -- delay time comes first even though it lives in
        // NonRTPC -- so this cannot be split into two sequential sub-reads.
        explicit AkDelayFXParams(FWwiseArchive& Ar)
        {
            NonRTPC.fDelayTime = Ar.Read<float>();
            RTPC.fFeedback = Ar.Read<float>() * 0.01f;
            RTPC.fWetDryMix = Ar.Read<float>() * 0.01f;
            RTPC.fOutputLevel = DbToLinear(Ar.Read<float>());
            RTPC.bFeedbackEnabled = Ar.Read<uint8_t>() != 0;
            NonRTPC.bProcessLFE = Ar.Read<uint8_t>() != 0;
        }
    };

    class CAkDelayFXParams : public IAkPluginParam
    {
    public:
        AkDelayFXParams Params;

        explicit CAkDelayFXParams(FWwiseArchive& Ar) : Params(Ar) {}
    };
}
