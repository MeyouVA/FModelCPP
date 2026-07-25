// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkTremoloFXParams.cs
#pragma once

#include <cstdint>

#include "../WwiseArchive.h"
#include "CAkFlangerFXParams.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    struct AkTremoloRTPCParams
    {
        float ModDepth = 0;
        DSPALLParams ModParams{};
        float OutputGain = 0;

        AkTremoloRTPCParams() = default;

        explicit AkTremoloRTPCParams(FWwiseArchive& Ar)
        {
            ModDepth = Ar.Read<float>() * 0.01f;
            ModParams.LfoParams.Frequency = Ar.Read<float>();
            ModParams.LfoParams.Waveform = Ar.Read<DSPLfoWaveform>();
            ModParams.LfoParams.Smooth = Ar.Read<float>() * 0.01f;
            ModParams.LfoParams.PWM = Ar.Read<float>() * 0.01f;
            ModParams.PhaseParams = Ar.Read<DSPPhaseParams>();
            OutputGain = DbToLinear(Ar.Read<float>());
        }
    };

    struct AkTremoloNonRTPCParams
    {
        bool ProcessCenter = false;
        bool ProcessLFE = false;
    };

    struct AkTremoloFXParams
    {
        AkTremoloRTPCParams RTPC;
        AkTremoloNonRTPCParams NonRTPC;

        AkTremoloFXParams() = default;

        explicit AkTremoloFXParams(FWwiseArchive& Ar)
        {
            RTPC = AkTremoloRTPCParams(Ar);
            NonRTPC.ProcessCenter = Ar.Read<uint8_t>() != 0;
            NonRTPC.ProcessLFE = Ar.Read<uint8_t>() != 0;
        }
    };

    class CAkTremoloFXParams : public IAkPluginParam
    {
    public:
        AkTremoloFXParams Params;

        explicit CAkTremoloFXParams(FWwiseArchive& Ar) : Params(Ar) {}
    };
}
