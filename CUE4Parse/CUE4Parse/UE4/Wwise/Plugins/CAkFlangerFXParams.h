// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkFlangerFXParams.cs
#pragma once

#include <cstdint>

#include "../WwiseArchive.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    enum class DSPLfoWaveform : uint32_t
    {
        First = 0x0,
        // C# guards this with #pragma warning disable CA1069 -- the duplicate is intentional, First and
        // Sine are the same value. C++ allows it silently, so it is pinned by test instead.
        Sine = 0x0,
        Triangle = 0x1,
        Square = 0x2,
        SawUp = 0x3,
        SawDown = 0x4,
        Num = 0x5
    };

    enum class DSPPhaseMode : uint32_t
    {
        LeftRight = 0x0,
        FrontRear = 0x1,
        Circular = 0x2,
        Random = 0x3
    };

    struct DSPLfoParams
    {
        float Frequency;
        DSPLfoWaveform Waveform;
        float Smooth;
        float PWM;
    };

#pragma pack(push, 4)
    struct DSPPhaseParams
    {
        float PhaseOffset;
        DSPPhaseMode PhaseMode;
        float PhaseSpread;
    };
#pragma pack(pop)

    struct DSPALLParams
    {
        DSPLfoParams LfoParams;
        DSPPhaseParams PhaseParams;
    };

    struct AkFlangerRTPCParams
    {
        float DryLevel = 0;
        float FfwdLevel = 0;
        float FbackLevel = 0;
        float ModDepth = 0;
        DSPALLParams ModParams{};
        float OutputLevel = 0;
        float WetDryMix = 0;
        bool HasChanged = false;

        AkFlangerRTPCParams() = default;

        explicit AkFlangerRTPCParams(FWwiseArchive& Ar)
        {
            DryLevel = Ar.Read<float>();
            FfwdLevel = Ar.Read<float>();
            FbackLevel = Ar.Read<float>();
            ModDepth = Ar.Read<float>() * 0.01f;
            ModParams.LfoParams.Frequency = Ar.Read<float>();
            ModParams.LfoParams.Waveform = Ar.Read<DSPLfoWaveform>();
            ModParams.LfoParams.Smooth = Ar.Read<float>() * 0.01f;
            ModParams.LfoParams.PWM = Ar.Read<float>() * 0.01f;
            ModParams.PhaseParams = Ar.Read<DSPPhaseParams>();
            OutputLevel = DbToLinear(Ar.Read<float>());
            WetDryMix = Ar.Read<float>();
        }
    };

    struct AkFlangerNonRTPCParams
    {
        float DelayTime = 0;
        bool EnableLFO = false;
        bool ProcessCenter = false;
        bool ProcessLFE = false;
        bool HasChanged = false;
    };

    struct AkFlangerFXParams
    {
        AkFlangerRTPCParams RTPC;
        AkFlangerNonRTPCParams NonRTPC;

        AkFlangerFXParams() = default;

        explicit AkFlangerFXParams(FWwiseArchive& Ar)
        {
            NonRTPC.DelayTime = Ar.Read<float>();
            RTPC = AkFlangerRTPCParams(Ar);
            NonRTPC.EnableLFO = Ar.Read<uint8_t>() != 0;
            NonRTPC.ProcessCenter = Ar.Read<uint8_t>() != 0;
            NonRTPC.ProcessLFE = Ar.Read<uint8_t>() != 0;
        }
    };

    class CAkFlangerFXParams : public IAkPluginParam
    {
    public:
        AkFlangerFXParams Params;

        explicit CAkFlangerFXParams(FWwiseArchive& Ar) : Params(Ar) {}
    };
}
