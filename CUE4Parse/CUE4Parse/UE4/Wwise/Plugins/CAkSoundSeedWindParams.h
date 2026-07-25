// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkSoundSeedWindParams.cs
#pragma once

#include <cstdint>
#include <vector>

#include "../WwiseArchive.h"
#include "../Objects/AkConversionTable.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    using CUE4Parse::UE4::Wwise::Objects::CAkConversionTable;

    struct FSoundSeedParamvalue
    {
        float BaseValue = 0;
        float RandomValue = 0;
        bool bAutomation = false;

        FSoundSeedParamvalue() = default;

        explicit FSoundSeedParamvalue(FWwiseArchive& Ar)
        {
            BaseValue = Ar.Read<float>();
            RandomValue = Ar.Read<float>();
            bAutomation = Ar.Read<uint8_t>() != 0;
        }
    };

    struct AkWindDeflectorParams
    {
        float Distance = 0;
        float Angle = 0;
        float Frequency = 0;
        float QFactor = 0;
        float Gain = 0;

        AkWindDeflectorParams() = default;

        explicit AkWindDeflectorParams(FWwiseArchive& Ar)
        {
            Distance = Ar.Read<float>();
            Angle = Ar.Read<float>();
            Frequency = Ar.Read<float>();
            QFactor = Ar.Read<float>();
            Gain = DbToLinear(Ar.Read<float>());
        }
    };

    struct AkWindParams
    {
        float Duration = 0;
        float DurationRandom = 0;
        uint32_t ChannelMask = 0;
        float MinDistance = 0;
        float AttenuationRolloff = 0;
        float MaxDistance = 0;
        float DynamicRange = 0;
        float PlaybackRate = 0;
        std::vector<FSoundSeedParamvalue> Values;

        AkWindParams() = default;

        // Note MaxDistance is *not* read here -- the caller reads it after the deflector count.
        explicit AkWindParams(FWwiseArchive& Ar)
        {
            Duration = Ar.Read<float>();
            DurationRandom = Ar.Read<float>();
            const uint16_t value = Ar.Read<uint16_t>();
            // 0 and 2 are sentinels standing for real channel masks, not counts.
            if (value == 0)      ChannelMask = 4;
            else if (value == 2) ChannelMask = 0x603;
            else                 ChannelMask = value;

            MinDistance = Ar.Read<float>();
            AttenuationRolloff = Ar.Read<float>();
            DynamicRange = Ar.Read<float>();
            PlaybackRate = Ar.Read<float>();
            Values = Ar.ReadArrayWith(7, [&Ar] { return FSoundSeedParamvalue(Ar); });
        }
    };

    class CAkSoundSeedWindParams : public IAkPluginParam
    {
    public:
        AkWindParams WindParams;
        std::vector<AkWindDeflectorParams> m_pDeflectors;
        std::vector<CAkConversionTable> m_Curves;

        explicit CAkSoundSeedWindParams(FWwiseArchive& Ar)
        {
            WindParams = AkWindParams(Ar);
            const uint16_t deflectorCount = Ar.Read<uint16_t>();
            WindParams.MaxDistance = Ar.Read<float>();
            m_pDeflectors = Ar.ReadArrayWith(deflectorCount, [&Ar] { return AkWindDeflectorParams(Ar); });
            // Each curve is preceded by an index that C# reads and discards.
            const int curveCount = Ar.Read<uint16_t>();
            m_Curves = Ar.ReadArrayWith(curveCount, [&Ar] {
                Ar.Read<int32_t>();
                return CAkConversionTable(Ar, false);
            });
        }
    };
}
