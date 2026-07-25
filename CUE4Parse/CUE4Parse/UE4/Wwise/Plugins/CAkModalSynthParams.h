// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkModalSynthParams.cs
#pragma once

#include <cstdint>
#include <vector>

#include "../WwiseArchive.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    struct AkModalSynthMode
    {
        float fFreq;
        float fMag;
        float fBW;
    };

    struct AkModalSynthParams
    {
        float fResidualLevel = 0;
        float fOutputLevel = 0;
        float fFreqAmt = 0;
        float fFreqVar = 0;
        float fBWAmt = 0;
        float fBWVar = 0;
        float fMagVar = 0;
        float fModelQuality = 0;

        bool bFreqEnable = false;
        bool bBWEnable = false;
        bool bMagEnable = false;

        AkModalSynthParams() = default;

        explicit AkModalSynthParams(FWwiseArchive& Ar)
        {
            fResidualLevel = DbToLinear(Ar.Read<float>());
            fOutputLevel = DbToLinear(Ar.Read<float>());
            fFreqAmt = Ar.Read<float>();
            fFreqVar = Ar.Read<float>();
            fBWAmt = Ar.Read<float>();
            fBWVar = Ar.Read<float>();
            fMagVar = Ar.Read<float>();
            fModelQuality = Ar.Read<float>();
            bFreqEnable = Ar.Read<uint8_t>() != 0;
            bBWEnable = Ar.Read<uint8_t>() != 0;
            bMagEnable = Ar.Read<uint8_t>() != 0;
        }
    };

    class CAkModalSynthParams : public IAkPluginParam
    {
    public:
        AkModalSynthParams Params;
        float m_fGlobalGain = 0;
        std::vector<AkModalSynthMode> m_pModes;

        explicit CAkModalSynthParams(FWwiseArchive& Ar)
        {
            Params = AkModalSynthParams(Ar);
            m_fGlobalGain = Ar.Read<float>();
            m_pModes = Ar.ReadArray<AkModalSynthMode>(Ar.Read<uint16_t>());
        }
    };
}
