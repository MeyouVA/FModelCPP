// Ported from CUE4Parse/UE4/Wwise/Plugins/iZotope/CiZTrashMultibandDistortionFXParams.cs
#pragma once

#include <cstdint>

#include "../../WwiseArchive.h"
#include "../IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins::iZotope
{
    // Ordering note: the blocks run Distortion1Band1, Distortion2Band1, Distortion1Band2, ... -- distortion
    // slot is the inner index, band the outer.
#pragma pack(push, 4)
    struct TrashMultibandDistortionFXParams
    {
        float Distortion1Band1InputGain;
        float Distortion1Band1Overdrive;
        float Distortion1Band1Trash;
        float Distortion1Band1Mix;
        float Distortion1Band1OutputGain;
        uint32_t Distortion1Band1Type;

        float Distortion2Band1InputGain;
        float Distortion2Band1Overdrive;
        float Distortion2Band1Trash;
        float Distortion2Band1Mix;
        float Distortion2Band1OutputGain;
        uint32_t Distortion2Band1Type;

        float Distortion1Band2InputGain;
        float Distortion1Band2Overdrive;
        float Distortion1Band2Trash;
        float Distortion1Band2Mix;
        float Distortion1Band2OutputGain;
        uint32_t Distortion1Band2Type;

        float Distortion2Band2InputGain;
        float Distortion2Band2Overdrive;
        float Distortion2Band2Trash;
        float Distortion2Band2Mix;
        float Distortion2Band2OutputGain;
        uint32_t Distortion2Band2Type;

        float Distortion1Band3InputGain;
        float Distortion1Band3Overdrive;
        float Distortion1Band3Trash;
        float Distortion1Band3Mix;
        float Distortion1Band3OutputGain;
        uint32_t Distortion1Band3Type;

        float Distortion2Band3InputGain;
        float Distortion2Band3Overdrive;
        float Distortion2Band3Trash;
        float Distortion2Band3Mix;
        float Distortion2Band3OutputGain;
        uint32_t Distortion2Band3Type;

        float MultibandCrossover1;
        float MultibandCrossover2;
    };
#pragma pack(pop)

    class CiZTrashMultibandDistortionFXParams : public IAkPluginParam
    {
    public:
        TrashMultibandDistortionFXParams Params;

        explicit CiZTrashMultibandDistortionFXParams(FWwiseArchive& Ar)
            : Params(Ar.Read<TrashMultibandDistortionFXParams>()) {}
    };
}
