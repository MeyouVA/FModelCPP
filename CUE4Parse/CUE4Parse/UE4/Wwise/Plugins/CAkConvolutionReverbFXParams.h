// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkConvolutionReverbFXParams.cs
#pragma once

#include <cstdint>

#include "../WwiseArchive.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    enum class AkConvolutionAlgoType : uint32_t
    {
        AKCONVALGOTYPE_DOWNMIX = 0x0,
        AKCONVALGOTYPE_DIRECT = 0x1
    };

    struct AkConvolutionReverbParams
    {
        float fPreDelay = 0;
        float fFrontRearDelay = 0;
        float fStereoWidth = 0;
        float fInputCenterLevel = 0;
        float fInputLFELevel = 0;
        float fInputStereoWidth = 0;
        float fFrontLevel = 0;
        float fRearLevel = 0;
        float fCenterLevel = 0;
        float fLFELevel = 0;
        float fDryLevel = 0;
        float fWetLevel = 0;
        AkConvolutionAlgoType eAlgoType = static_cast<AkConvolutionAlgoType>(0);
        float fInputThreshold = 0;
        uint8_t unknown = 0;

        AkConvolutionReverbParams() = default;

        explicit AkConvolutionReverbParams(FWwiseArchive& Ar)
        {
            fPreDelay = Ar.Read<float>();
            fFrontRearDelay = Ar.Read<float>();
            fStereoWidth = Ar.Read<float>();
            fInputCenterLevel = DbToLinear(Ar.Read<float>());
            fInputLFELevel = DbToLinear(Ar.Read<float>());
            // Inserted mid-struct at 120 -- everything after it shifts on older banks.
            fInputStereoWidth = Ar.Version >= 120 ? Ar.Read<float>() : 0;
            fFrontLevel = DbToLinear(Ar.Read<float>());
            fRearLevel = DbToLinear(Ar.Read<float>());
            fCenterLevel = DbToLinear(Ar.Read<float>());
            fLFELevel = DbToLinear(Ar.Read<float>());
            fDryLevel = DbToLinear(Ar.Read<float>());
            fWetLevel = DbToLinear(Ar.Read<float>());
            eAlgoType = Ar.Read<AkConvolutionAlgoType>();
            fInputThreshold = Ar.Version >= 135 ? DbToLinear(Ar.Read<float>()) : 0;
            unknown = Ar.Version >= 145 ? Ar.Read<uint8_t>() : static_cast<uint8_t>(0);
        }
    };

    class CAkConvolutionReverbFXParams : public IAkPluginParam
    {
    public:
        AkConvolutionReverbParams Params;

        explicit CAkConvolutionReverbFXParams(FWwiseArchive& Ar) : Params(Ar) {}
    };
}
