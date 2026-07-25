// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkCompressorFXParams.cs
#pragma once

#include <cstdint>

#include "../WwiseArchive.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    struct AkCompressorFXParams
    {
        float Threshold = 0;
        float Ratio = 0;
        float Attack = 0;
        float Release = 0;
        float OutputLevel = 0;
        float ChannelLinkPercentage = 0;
        bool ProcessLFE = false;
        bool ChannelLink = false;
        bool SidechainGlobalScope = false;
        uint32_t SidechainId = 0;

        AkCompressorFXParams() = default;

        explicit AkCompressorFXParams(FWwiseArchive& Ar)
        {
            Threshold = Ar.Read<float>();
            Ratio = Ar.Read<float>();
            Attack = Ar.Read<float>();
            Release = Ar.Read<float>();
            OutputLevel = DbToLinear(Ar.Read<float>());
            ChannelLinkPercentage = Ar.Version >= 172 ? Ar.Read<float>() : 0;
            ProcessLFE = Ar.Read<uint8_t>() != 0;
            ChannelLink = Ar.Read<uint8_t>() != 0;
            // C# short-circuits: below 172 no byte is consumed at all.
            SidechainGlobalScope = Ar.Version >= 172 && Ar.Read<uint8_t>() != 0;
            SidechainId = Ar.Version >= 172 ? Ar.Read<uint32_t>() : 0;
        }
    };

    class CAkCompressorFXParams : public IAkPluginParam
    {
    public:
        AkCompressorFXParams Params;

        explicit CAkCompressorFXParams(FWwiseArchive& Ar) : Params(Ar) {}
    };
}
