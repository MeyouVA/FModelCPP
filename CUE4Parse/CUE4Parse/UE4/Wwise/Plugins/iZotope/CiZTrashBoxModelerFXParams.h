// Ported from CUE4Parse/UE4/Wwise/Plugins/iZotope/CiZTrashBoxModelerFXParams.cs
#pragma once

#include <cstdint>

#include "../../WwiseArchive.h"
#include "../IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins::iZotope
{
#pragma pack(push, 4)
    struct iZTrashBoxModelerNonRTPCParams
    {
        uint32_t uBoxModel;
        uint32_t uMicType;
    };

    struct iZTrashBoxModelerRTPCParams
    {
        float fInputGain;
        float fOutputGain;
        float fMix;
        float fTrim;
        float fLength;
    };
#pragma pack(pop)

    struct iZTrashBoxModelerFXParams
    {
        iZTrashBoxModelerNonRTPCParams NonRTPC{};
        iZTrashBoxModelerRTPCParams RTPC{};

        iZTrashBoxModelerFXParams() = default;

        explicit iZTrashBoxModelerFXParams(FWwiseArchive& Ar)
            : NonRTPC(Ar.Read<iZTrashBoxModelerNonRTPCParams>()), RTPC(Ar.Read<iZTrashBoxModelerRTPCParams>()) {}
    };

    class CiZTrashBoxModelerFXParams : public IAkPluginParam
    {
    public:
        iZTrashBoxModelerFXParams Params;

        explicit CiZTrashBoxModelerFXParams(FWwiseArchive& Ar) : Params(Ar) {}
    };
}
