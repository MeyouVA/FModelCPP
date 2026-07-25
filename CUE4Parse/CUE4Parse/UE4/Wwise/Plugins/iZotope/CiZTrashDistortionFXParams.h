// Ported from CUE4Parse/UE4/Wwise/Plugins/iZotope/CiZTrashDistortionFXParams.cs
#pragma once

#include <cstdint>

#include "../../WwiseArchive.h"
#include "../IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins::iZotope
{
#pragma pack(push, 4)
    struct iZTrashDistortionRTPCParams
    {
        float Distortion1InputGain;
        float Distortion1Overdrive;
        float Distortion1Trash;
        float Distortion1Mix;
        float Distortion1OutputGain;
        uint32_t Distortion1Type;
        float Distortion2InputGain;
        float Distortion2Overdrive;
        float Distortion2Trash;
        float Distortion2Mix;
        float Distortion2OutputGain;
        uint32_t Distortion2Type;
    };
#pragma pack(pop)

    struct iZTrashDistortionFXParams
    {
        iZTrashDistortionRTPCParams RTPC{};

        iZTrashDistortionFXParams() = default;

        explicit iZTrashDistortionFXParams(FWwiseArchive& Ar) : RTPC(Ar.Read<iZTrashDistortionRTPCParams>()) {}
    };

    class CiZTrashDistortionFXParams : public IAkPluginParam
    {
    public:
        iZTrashDistortionFXParams Params;

        explicit CiZTrashDistortionFXParams(FWwiseArchive& Ar) : Params(Ar) {}
    };
}
