// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkFxSrcAudioInputParams.cs
#pragma once

#include "../WwiseArchive.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
#pragma pack(push, 4)
    struct AkFXSrcAudioInputParams
    {
        float fGain;
    };
#pragma pack(pop)

    // C# marks the wrapper `internal`; C++ has no equivalent, so it is public here.
    class CAkFxSrcAudioInputParams : public IAkPluginParam
    {
    public:
        AkFXSrcAudioInputParams Params;

        explicit CAkFxSrcAudioInputParams(FWwiseArchive& Ar) : Params(Ar.Read<AkFXSrcAudioInputParams>()) {}
    };
}
