// Ported from CUE4Parse/UE4/Wwise/Plugins/iZotope/CiZTrashFiltersFXParams.cs
#pragma once

#include <cstdint>

#include "../../WwiseArchive.h"
#include "../IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins::iZotope
{
    // Three identical filter blocks, flattened in C# rather than expressed as an array; kept flat.
#pragma pack(push, 4)
    struct iZTrashFiltersFXParams
    {
        uint32_t Filter1Type;
        float Filter1Frequency;
        float Filter1Q;
        float Filter1Resonance;
        float Filter1Gain;
        uint32_t Filter1Trigger;
        uint32_t Filter1LFOType;
        float Filter1LFOPeriod;
        float Filter1LFOTargetFreq;
        float Filter1LFOTargetGain;
        float Filter1LFOTargetQ;
        float Filter1LFOTargetRes;

        uint32_t Filter2Type;
        float Filter2Frequency;
        float Filter2Q;
        float Filter2Resonance;
        float Filter2Gain;
        uint32_t Filter2Trigger;
        uint32_t Filter2LFOType;
        float Filter2LFOPeriod;
        float Filter2LFOTargetFreq;
        float Filter2LFOTargetGain;
        float Filter2LFOTargetQ;
        float Filter2LFOTargetRes;

        uint32_t Filter3Type;
        float Filter3Frequency;
        float Filter3Q;
        float Filter3Resonance;
        float Filter3Gain;
        uint32_t Filter3Trigger;
        uint32_t Filter3LFOType;
        float Filter3LFOPeriod;
        float Filter3LFOTargetFreq;
        float Filter3LFOTargetGain;
        float Filter3LFOTargetQ;
        float Filter3LFOTargetRes;
    };
#pragma pack(pop)

    class CiZTrashFiltersFXParams : public IAkPluginParam
    {
    public:
        iZTrashFiltersFXParams Params;

        explicit CiZTrashFiltersFXParams(FWwiseArchive& Ar) : Params(Ar.Read<iZTrashFiltersFXParams>()) {}
    };
}
