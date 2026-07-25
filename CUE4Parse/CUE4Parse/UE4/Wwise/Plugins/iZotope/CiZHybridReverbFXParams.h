// Ported from CUE4Parse/UE4/Wwise/Plugins/iZotope/CiZHybridReverbFXParams.cs
#pragma once

#include <cstdint>

#include "../../WwiseArchive.h"
#include "../IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins::iZotope
{
#pragma pack(push, 4)
    struct iZHybridReverbNonRTPCParams
    {
        float fDecayTime;
        float fLowFreq;
        float fHighFreq;
        float fLowRatio;
        float fMidRatio;
        float fHighRatio;
        uint32_t uQuality;
    };

    struct iZHybridReverbRTPCParams
    {
        float fEarlyGain;
        float fTailGain;
        float fPreDelayFront;
        float fPreDelayRear;
        float fFrontWet;
        float fFrontDry;
        float fRearWet;
        float fRearDry;
    };
#pragma pack(pop)

    // Note the non-RTPC block comes first on the wire.
    struct iZHybridReverbFXParams
    {
        iZHybridReverbNonRTPCParams NonRTPC{};
        iZHybridReverbRTPCParams RTPC{};

        iZHybridReverbFXParams() = default;

        explicit iZHybridReverbFXParams(FWwiseArchive& Ar)
            : NonRTPC(Ar.Read<iZHybridReverbNonRTPCParams>()), RTPC(Ar.Read<iZHybridReverbRTPCParams>()) {}
    };

    class CiZHybridReverbFXParams : public IAkPluginParam
    {
    public:
        iZHybridReverbFXParams Params;

        explicit CiZHybridReverbFXParams(FWwiseArchive& Ar) : Params(Ar) {}
    };
}
