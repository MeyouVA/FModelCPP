// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkFxSrcSilenceParams.cs
#pragma once

#include "../WwiseArchive.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
#pragma pack(push, 4)
    struct AkFxSrcSilenceParams
    {
        float fDuration;
        float fRandomizedLengthMinus;
        float fRandomizedLengthPlus;
    };
#pragma pack(pop)

    class CAkFxSrcSilenceParams : public IAkPluginParam
    {
    public:
        AkFxSrcSilenceParams Params;

        explicit CAkFxSrcSilenceParams(FWwiseArchive& Ar) : Params(Ar.Read<AkFxSrcSilenceParams>()) {}
    };
}
