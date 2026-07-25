// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkMotionGeneratorParams.cs
#pragma once

#include <cstdint>
#include <vector>

#include "../WwiseArchive.h"
#include "../Objects/AkConversionTable.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    using CUE4Parse::UE4::Wwise::Objects::CAkConversionTable;

    struct AkMotionGeneratorParams
    {
        float m_fPeriod = 0;
        float m_fPeriodMultiplier = 0;
        float m_fDuration = 0;
        float m_fAttackTime = 0;
        float m_fDecayTime = 0;
        float m_fSustainTime = 0;
        float m_fReleaseTime = 0;
        float m_fSustainLevel = 0;

        uint16_t m_eDurationType = 0;
        std::vector<CAkConversionTable> m_Curves;

        AkMotionGeneratorParams() = default;

        explicit AkMotionGeneratorParams(FWwiseArchive& Ar)
        {
            m_fPeriod = Ar.Read<float>();
            m_fPeriodMultiplier = Ar.Read<float>();
            m_fDuration = Ar.Read<float>();
            m_fAttackTime = Ar.Read<float>();
            m_fDecayTime = Ar.Read<float>();
            m_fSustainTime = Ar.Read<float>();
            m_fReleaseTime = Ar.Read<float>();
            m_fSustainLevel = DbToLinear(Ar.Read<float>());
            m_eDurationType = Ar.Read<uint16_t>();
            // readScaling: false -- these curves carry no scaling byte.
            const int curveCount = Ar.Read<uint16_t>();
            m_Curves = Ar.ReadArrayWith(curveCount, [&Ar] { return CAkConversionTable(Ar, false); });
        }
    };

    class CAkMotionGeneratorParams : public IAkPluginParam
    {
    public:
        AkMotionGeneratorParams Params;

        explicit CAkMotionGeneratorParams(FWwiseArchive& Ar) : Params(Ar) {}
    };
}
