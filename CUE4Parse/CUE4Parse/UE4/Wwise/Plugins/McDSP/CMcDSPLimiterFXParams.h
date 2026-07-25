// Ported from CUE4Parse/UE4/Wwise/Plugins/McDSP/CMcDSPLimiterFXParams.cs
#pragma once

#include <cstdint>

#include "../../WwiseArchive.h"
#include "../IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins::McDSP
{
    enum class LimiterCharacterType : int32_t
    {
        eCharacterMode_Clean = 0x0,
        eCharacterMode_Soft = 0x1,
        eCharacterMode_Smart = 0x2,
        eCharacterMode_Dynamic = 0x3,
        eCharacterMode_Loud = 0x4,
        eCharacterMode_Crush = 0x5
    };

#pragma pack(push, 4)
    struct McDSPLimiterFXParams
    {
        float fCeiling;
        float fThreshold;
        float fKnee;
        float fRelease;
        LimiterCharacterType eMode;
    };
#pragma pack(pop)

    class CMcDSPLimiterFXParams : public IAkPluginParam
    {
    public:
        McDSPLimiterFXParams Params;

        explicit CMcDSPLimiterFXParams(FWwiseArchive& Ar) : Params(Ar.Read<McDSPLimiterFXParams>()) {}
    };
}
