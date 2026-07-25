// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkFxSrcSineParams.cs
#pragma once

#include <cstdint>

#include "../WwiseArchive.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    // Unlike the silence params this one is read field by field (the gain is converted), so the
    // [StructLayout] on the C# side is decorative here.
    struct AkFxSrcSineParams
    {
        float fFrequency = 0;
        float fGain = 0;
        float fDuration = 0;
        uint32_t uChannelMask = 0;

        AkFxSrcSineParams() = default;

        explicit AkFxSrcSineParams(FWwiseArchive& Ar)
        {
            fFrequency = Ar.Read<float>();
            fGain = DbToLinear(Ar.Read<float>());
            fDuration = Ar.Read<float>();
            uChannelMask = Ar.Read<uint32_t>();
        }
    };

    class CAkFxSrcSineParams : public IAkPluginParam
    {
    public:
        AkFxSrcSineParams Params;

        explicit CAkFxSrcSineParams(FWwiseArchive& Ar) : Params(Ar) {}
    };
}
