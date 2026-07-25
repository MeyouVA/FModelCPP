// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkMeterFXParams.cs
#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "../WwiseArchive.h"
#include "../Enums/EAkChannelConfig.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    using CUE4Parse::UE4::Wwise::Enums::EAkChannelConfig;

    enum class AkMeterMode : uint8_t
    {
        Peak = 0x0,
        RMS = 0x1
    };

    enum class AkMeterScope : uint8_t
    {
        Global = 0,
        GameObject = 1
    };

    struct AkMeterFXParams
    {
        struct AkMeterRTPCParams
        {
            float fAttack = 0;
            float fRelease = 0;
            float fMin = 0;
            float fMax = 0;
            float fHold = 0;
            bool bInfiniteHold = false;

            AkMeterRTPCParams() = default;

            explicit AkMeterRTPCParams(FWwiseArchive& Ar)
            {
                fAttack = Ar.Read<float>();
                fRelease = Ar.Read<float>();
                fMin = Ar.Read<float>();
                fMax = Ar.Read<float>();
                fHold = Ar.Read<float>();
                // C# short-circuits: below 144 no byte is consumed.
                bInfiniteHold = Ar.Version >= 144 && Ar.Read<uint8_t>() != 0;
            }
        };

        struct AkMeterNonRTPCParams
        {
            std::optional<AkMeterMode> eMode;
            std::optional<AkMeterScope> eScope;
            bool bApplyDownstreamVolume = false;
            uint32_t uGameParamID = 0;

            AkMeterNonRTPCParams() = default;

            explicit AkMeterNonRTPCParams(FWwiseArchive& Ar)
            {
                if (Ar.Version > 88) eMode = Ar.Read<AkMeterMode>();
                if (Ar.Version >= 125) eScope = Ar.Read<AkMeterScope>();
                bApplyDownstreamVolume = Ar.Version > 113 && Ar.Read<uint8_t>() != 0;
                uGameParamID = Ar.Read<uint32_t>();
            }
        };

        AkMeterRTPCParams RTPC;
        AkMeterNonRTPCParams NonRTPC;

        AkMeterFXParams() = default;

        explicit AkMeterFXParams(FWwiseArchive& Ar) : RTPC(Ar), NonRTPC(Ar) {}
    };

    struct AkMeterParams
    {
        AkMeterMode eMode = static_cast<AkMeterMode>(0);
        AkMeterScope eScope = static_cast<AkMeterScope>(0);
        EAkChannelConfig mixdownCfg = static_cast<EAkChannelConfig>(0);
        bool bApplyDownstreamVolume = false;
        bool bInfiniteHold = false;

        AkMeterParams() = default;

        explicit AkMeterParams(FWwiseArchive& Ar)
        {
            eMode = Ar.Read<AkMeterMode>();
            eScope = Ar.Read<AkMeterScope>();
            mixdownCfg = Ar.Read<EAkChannelConfig>();
            bApplyDownstreamVolume = Ar.Read<uint8_t>() != 0;
            bInfiniteHold = Ar.Read<uint8_t>() != 0;
        }
    };

    struct AkMeterBallisticParams
    {
        uint32_t uGameParamID = 0;
        float fAttack = 0;
        float fRelease = 0;
        float fMin = 0;
        float fMax = 0;
        float fHold = 0;

        AkMeterBallisticParams() = default;

        explicit AkMeterBallisticParams(FWwiseArchive& Ar)
        {
            uGameParamID = Ar.Read<uint32_t>();
            fAttack = Ar.Read<float>();
            fRelease = Ar.Read<float>();
            fMin = Ar.Read<float>();
            fMax = Ar.Read<float>();
            fHold = Ar.Read<float>();
        }
    };

    struct AkMultibandMeterBandParams
    {
        bool bFilterEnabled = false;
        uint8_t uNumCascadesLow = 0;
        uint8_t uNumCascadesHigh = 0;
        float fFrequencyLow = 0;
        float fFrequencyHigh = 0;

        AkMultibandMeterBandParams() = default;

        explicit AkMultibandMeterBandParams(FWwiseArchive& Ar)
        {
            bFilterEnabled = Ar.Read<uint8_t>() != 0;
            uNumCascadesLow = Ar.Read<uint8_t>();
            uNumCascadesHigh = Ar.Read<uint8_t>();
            fFrequencyLow = Ar.Read<float>();
            fFrequencyHigh = Ar.Read<float>();
        }
    };

    // Past 154 the layout splits into a params/ballistics pair; before that it is the single legacy struct.
    // Exactly one of the two shapes is populated.
    class CAkMeterFXParams : public IAkPluginParam
    {
    public:
        std::optional<AkMeterFXParams> Params;
        std::optional<AkMeterBallisticParams> BallisticParams;
        std::optional<AkMeterParams> MeterParams;

        explicit CAkMeterFXParams(FWwiseArchive& Ar)
        {
            if (Ar.Version > 154)
            {
                MeterParams = AkMeterParams(Ar);
                BallisticParams = AkMeterBallisticParams(Ar);
            }
            else
            {
                Params = AkMeterFXParams(Ar);
            }
        }
    };

    class CAkMultibandMeterFXParams : public IAkPluginParam
    {
    public:
        AkMeterParams MeterParams;
        std::vector<AkMeterBallisticParams> BallisticParams;
        // C#'s field name has the doubled 's'; kept.
        std::vector<AkMultibandMeterBandParams> BandParamss;

        explicit CAkMultibandMeterFXParams(FWwiseArchive& Ar)
            : MeterParams(Ar),
              BallisticParams(Ar.ReadArrayWith(5, [&Ar] { return AkMeterBallisticParams(Ar); })),
              BandParamss(Ar.ReadArrayWith(5, [&Ar] { return AkMultibandMeterBandParams(Ar); })) {}
    };
}
