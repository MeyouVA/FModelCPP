// Ported from CUE4Parse/UE4/Wwise/Plugins/atmoky/CatmokyEarsFXParams.cs
// Namespace keeps C#'s lowercase 'atmoky' spelling.
#pragma once

#include <cstdint>

#include "../../WwiseArchive.h"
#include "../IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins::atmoky
{
    class CAtmokyEarsFXParams : public IAkPluginParam
    {
    public:
        float ExternalizerAmount;
        float PersonalizationSize;
        int32_t ExternalizerCharacter;
        uint8_t PerformanceMode;

        explicit CAtmokyEarsFXParams(FWwiseArchive& Ar)
            : ExternalizerAmount(Ar.Read<float>()),
              PersonalizationSize(Ar.Read<float>()),
              ExternalizerCharacter(Ar.Read<int32_t>()),
              PerformanceMode(Ar.Read<uint8_t>()) {}
    };
}
