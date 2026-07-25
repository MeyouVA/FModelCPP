// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkImpacterParams.cs
#pragma once

#include <cstdint>

#include "../WwiseArchive.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
#pragma pack(push, 4)
    struct AkImpacterParams
    {
        float Mass;
        float Velocity;
        float MinDuration;
        float ImpactPosition;
        float FMDepth;
        int32_t NumFiles;
        uint64_t ExcitationMask;
        uint64_t ModelMask;
        float OutputLevel;
        float MassRandom;
        float VelocityRandom;
        float ImpactPositionRandom;
        float FMDepthRandom;
    };
#pragma pack(pop)

    class CAkImpacterParams : public IAkPluginParam
    {
    public:
        AkImpacterParams Params;

        explicit CAkImpacterParams(FWwiseArchive& Ar) : Params(Ar.Read<AkImpacterParams>()) {}
    };
}
