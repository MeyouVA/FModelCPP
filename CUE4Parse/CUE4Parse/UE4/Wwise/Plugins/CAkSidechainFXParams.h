// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkSidechainFXParams.cs
#pragma once

#include <cstdint>

#include "../WwiseArchive.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    struct AkSidechainRTPCParams
    {
        float Volume = 0;
        int16_t LpfFactor = 0;
        int16_t HpfFactor = 0;
    };

    struct AkSidechainNonRTPCParams
    {
        uint32_t SidechainId = 0;
        int32_t SidechainScope = 0;
        bool bDelayOutput = false;
    };

    struct AkSidechainSendFXParams
    {
        AkSidechainRTPCParams RTPC;
        AkSidechainNonRTPCParams NonRTPC;

        AkSidechainSendFXParams() = default;

        explicit AkSidechainSendFXParams(FWwiseArchive& Ar)
        {
            NonRTPC.SidechainId = Ar.Read<uint32_t>();
            NonRTPC.SidechainScope = Ar.Read<int32_t>();
            RTPC.Volume = DbToLinear(Ar.Read<float>());
            RTPC.LpfFactor = Ar.Read<int16_t>();
            RTPC.HpfFactor = Ar.Read<int16_t>();
            NonRTPC.bDelayOutput = Ar.Read<uint8_t>() != 0;
        }
    };

    // Identical to the send except the trailing bDelayOutput byte, which only the send reads.
    struct AkSidechainRecvFXParams
    {
        AkSidechainRTPCParams RTPC;
        AkSidechainNonRTPCParams NonRTPC;

        AkSidechainRecvFXParams() = default;

        explicit AkSidechainRecvFXParams(FWwiseArchive& Ar)
        {
            NonRTPC.SidechainId = Ar.Read<uint32_t>();
            NonRTPC.SidechainScope = Ar.Read<int32_t>();
            RTPC.Volume = DbToLinear(Ar.Read<float>());
            RTPC.LpfFactor = Ar.Read<int16_t>();
            RTPC.HpfFactor = Ar.Read<int16_t>();
        }
    };

    class CAkSidechainSendFXParams : public IAkPluginParam
    {
    public:
        AkSidechainSendFXParams Params;

        explicit CAkSidechainSendFXParams(FWwiseArchive& Ar) : Params(Ar) {}
    };

    class CAkSidechainRecvFXParams : public IAkPluginParam
    {
    public:
        AkSidechainRecvFXParams Params;

        explicit CAkSidechainRecvFXParams(FWwiseArchive& Ar) : Params(Ar) {}
    };
}
