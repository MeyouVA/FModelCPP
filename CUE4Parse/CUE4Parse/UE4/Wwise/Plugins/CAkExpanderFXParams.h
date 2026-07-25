// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkExpanderFXParams.cs
#pragma once

#include <cstdint>

#include "../WwiseArchive.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    struct AkExpanderParams
    {
        float fThreshold = 0;
        float fRatio = 0;
        float fAttack = 0;
        float fRelease = 0;
        float fOutputLevel = 0;
        bool bProcessLFE = false;
        bool bChannelLink = false;

        AkExpanderParams() = default;

        explicit AkExpanderParams(FWwiseArchive& Ar)
        {
            fThreshold = Ar.Read<float>();
            fRatio = Ar.Read<float>();
            fAttack = Ar.Read<float>();
            fRelease = Ar.Read<float>();
            fOutputLevel = DbToLinear(Ar.Read<float>());
            bProcessLFE = Ar.Read<uint8_t>() != 0;
            bChannelLink = Ar.Read<uint8_t>() != 0;
        }
    };

    class CAkExpanderFXParams : public IAkPluginParam
    {
    public:
        AkExpanderParams Params;

        explicit CAkExpanderFXParams(FWwiseArchive& Ar) : Params(Ar) {}
    };
}
