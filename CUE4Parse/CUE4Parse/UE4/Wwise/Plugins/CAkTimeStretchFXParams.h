// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkTimeStretchFXParams.cs
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "../WwiseArchive.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    enum class StereoProcType : int32_t
    {
        Stereo = 0x0,
        CenterCut = 0x1
    };

    struct AkTimeStretchFXParams
    {
        uint32_t uWindowSize = 0;
        float fTimeStretch = 0;
        float fTimeStretchRandom = 0;
        float fPitchShift = 0;
        float fPitchShiftRandom = 0;
        float fOutputGain = 0;
        float fTolerance = 0;
        StereoProcType iStereoProc = static_cast<StereoProcType>(0);

        AkTimeStretchFXParams() = default;

        explicit AkTimeStretchFXParams(FWwiseArchive& Ar)
        {
            uWindowSize = Ar.Read<uint32_t>();
            fTimeStretch = Ar.Read<float>();
            fTimeStretchRandom = Ar.Read<float>();
            if (Ar.Version < 145)
            {
                fOutputGain = DbToLinear(Ar.Read<float>());
            }
            else
            {
                fPitchShift = Ar.Read<float>();
                fPitchShiftRandom = Ar.Read<float>();
                fOutputGain = DbToLinear(Ar.Read<float>());
                fTolerance = std::max(std::exp(Ar.Read<float>() * -0.0599999987f) - 0.00247884f, 0.0001f);
                iStereoProc = Ar.Read<StereoProcType>();
            }
        }
    };

    class CAkTimeStretchFXParams : public IAkPluginParam
    {
    public:
        AkTimeStretchFXParams Params;

        explicit CAkTimeStretchFXParams(FWwiseArchive& Ar) : Params(Ar) {}
    };
}
