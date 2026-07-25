// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkMotionSourceParams.cs
#pragma once

#include <cstdint>
#include <vector>

#include "../WwiseArchive.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    enum class EAkMotionSourceCurveType : uint8_t
    {
        Amplitude = 0x00,
        HapticsWave = 0x01
    };

    struct AkMotionSourceParams
    {
        float m_fChannel1 = 0;
        float m_fChannel2 = 0;
        float m_fChannel3 = 0;
        float m_fChannel4 = 0;
        float m_fChannel5 = 0;
        float m_fChannel6 = 0;
        float m_fChannel7 = 0;
        float m_fChannel8 = 0;
        uint8_t m_uNumCurves = 0;
        EAkMotionSourceCurveType m_uCurveType = static_cast<EAkMotionSourceCurveType>(0);
        std::vector<uint16_t> m_uAssigns;

        AkMotionSourceParams() = default;

        explicit AkMotionSourceParams(FWwiseArchive& Ar)
        {
            m_fChannel1 = Ar.Read<float>();
            m_fChannel2 = Ar.Read<float>();
            m_fChannel3 = Ar.Read<float>();
            m_fChannel4 = Ar.Read<float>();
            m_fChannel5 = Ar.Read<float>();
            m_fChannel6 = Ar.Read<float>();
            m_fChannel7 = Ar.Read<float>();
            m_fChannel8 = Ar.Read<float>();
            m_uNumCurves = Ar.Read<uint8_t>();
            m_uCurveType = Ar.Read<EAkMotionSourceCurveType>();
            m_uAssigns = Ar.ReadArray<uint16_t>(m_uNumCurves);
        }
    };

    class CAkMotionSourceParams : public IAkPluginParam
    {
    public:
        AkMotionSourceParams Params;

        explicit CAkMotionSourceParams(FWwiseArchive& Ar) : Params(Ar) {}
    };
}
