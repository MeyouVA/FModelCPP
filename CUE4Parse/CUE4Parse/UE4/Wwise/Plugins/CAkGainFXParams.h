// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkGainFXParams.cs
#pragma once

#include "../WwiseArchive.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
#pragma pack(push, 4)
    struct AkGainFXParams
    {
        float fFullbandGain;
        float fLFEGain;
    };
#pragma pack(pop)

    class CAkGainFXParams : public IAkPluginParam
    {
    public:
        AkGainFXParams Params;

        explicit CAkGainFXParams(FWwiseArchive& Ar) : Params(Ar.Read<AkGainFXParams>()) {}
    };
}
