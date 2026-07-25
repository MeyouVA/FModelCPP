// Ported from CUE4Parse/UE4/Wwise/Plugins/iZotope/CiZTrashDelayFXParams.cs
#pragma once

#include <cstdint>

#include "../../WwiseArchive.h"
#include "../IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins::iZotope
{
#pragma pack(push, 4)
    struct iZTrashDelayRTPCParams
    {
        float fDryOut;
        float fWetOut;
        float fLowCutoff;
        float fLowQ;
        float fHighCutoff;
        float fHighQ;
        float fAmount;
        float fFeedback;
        float fTrash;
        uint32_t uDelayType;
    };
#pragma pack(pop)

    struct iZTrashDelayFXParams
    {
        iZTrashDelayRTPCParams RTPC{};

        iZTrashDelayFXParams() = default;

        explicit iZTrashDelayFXParams(FWwiseArchive& Ar) : RTPC(Ar.Read<iZTrashDelayRTPCParams>()) {}
    };

    class CiZTrashDelayFXParams : public IAkPluginParam
    {
    public:
        iZTrashDelayFXParams Params;

        explicit CiZTrashDelayFXParams(FWwiseArchive& Ar) : Params(Ar) {}
    };
}
