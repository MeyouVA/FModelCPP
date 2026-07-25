// Ported from CUE4Parse/UE4/Wwise/Plugins/iZotope/CiZTrashDynamicsFXParams.cs
#pragma once

#include <cstdint>

#include "../../WwiseArchive.h"
#include "../IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins::iZotope
{
#pragma pack(push, 4)
    struct iZTrashDynamicsRTPCParams
    {
        uint32_t uBypass;
        float fCompressorThreshold;
        float fCompressorRatio;
        float fCompressorAttack;
        float fCompressorRelease;
        float fGateThreshold;
        float fGateRatio;
        float fGateAttack;
        float fGateRelease;
        float fGain;
    };
#pragma pack(pop)

    struct iZTrashDynamicsFXParams
    {
        iZTrashDynamicsRTPCParams RTPC{};

        iZTrashDynamicsFXParams() = default;

        explicit iZTrashDynamicsFXParams(FWwiseArchive& Ar) : RTPC(Ar.Read<iZTrashDynamicsRTPCParams>()) {}
    };

    class CiZTrashDynamicsFXParams : public IAkPluginParam
    {
    public:
        iZTrashDynamicsFXParams Params;

        explicit CiZTrashDynamicsFXParams(FWwiseArchive& Ar) : Params(Ar) {}
    };
}
