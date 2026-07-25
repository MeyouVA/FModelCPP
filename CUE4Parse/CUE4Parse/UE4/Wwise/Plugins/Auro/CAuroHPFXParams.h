// Ported from CUE4Parse/UE4/Wwise/Plugins/Auro/CAuroHPFXParams.cs
#pragma once

#include <cstdint>
#include <vector>

#include "../../WwiseArchive.h"
#include "../IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins::Auro
{
    struct AuroHPFXParams
    {
        std::vector<float> fParams;
        bool bBypass = false;
        bool bEnableReverb = false;

        AuroHPFXParams() = default;

        explicit AuroHPFXParams(FWwiseArchive& Ar)
        {
            fParams = Ar.ReadArray<float>(17);
            bBypass = Ar.Read<uint8_t>() != 0;
            bEnableReverb = Ar.Read<uint8_t>() != 0;
        }
    };

    class CAuroHPFXParams : public IAkPluginParam
    {
    public:
        AuroHPFXParams Params;

        explicit CAuroHPFXParams(FWwiseArchive& Ar) : Params(Ar) {}
    };
}
