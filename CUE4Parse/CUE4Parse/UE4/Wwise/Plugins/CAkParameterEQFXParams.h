// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkParameterEQFXParams.cs
#pragma once

#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

#include "../WwiseArchive.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    enum class AkFilterTypeOld : uint32_t
    {
        LowShelf = 0x0,
        PeakingEQ = 0x1,
        HighShelf = 0x2,
        LowPass = 0x3,
        HighPass = 0x4,
        BandPass = 0x5,
        Notch = 0x6
    };

    // Note the byte-backed modern enum uses an entirely different numbering from AkFilterTypeOld -- a
    // value is not interchangeable between the two.
    enum class AkFilterType : uint8_t
    {
        LowPass = 0x0,
        HighPass = 0x1,
        BandPass = 0x2,
        Notch = 0x3,
        LowShelf = 0x4,
        HighShelf = 0x5,
        PeakingEQ = 0x6,
        LowPassQ = 0x7,
        HighPassQ = 0x8
    };

#pragma pack(push, 1)
    struct AkFilterParams
    {
        AkFilterTypeOld FilterType;
        float FilterGain;
        float FilterFrequency;
        float FilterQFactor;
    };

    struct EQModuleParamsDynamic
    {
        float BandDynamicsThresholdDb;
        float BandDynamicsRangeDb;
        float BandDynamicsAttackMs;
        float BandDynamicsReleaseMs;
    };

    struct EQModuleParamsStatic
    {
        AkFilterType FilterType;
        uint8_t BandRolloff;
        float Frequency;
        float GainDb;
        float QFactor;
    };

    struct EQModuleParams
    {
        EQModuleParamsStatic Static;
        EQModuleParamsDynamic Dynamic;
    };
#pragma pack(pop)

    // Shared by the EQ and the guitar distortion; declared in CAkGuitarDistortionFXParams.cs on the C#
    // side but needed here first, so it lives in this header. Same namespace either way.
#pragma pack(push, 1)
    struct AkFilterBand
    {
        AkFilterTypeOld FilterType;
        float Gain;
        float Frequency;
        float QFactor;
        bool OnOff;
    };
#pragma pack(pop)

    class IAkParametricEQFXParams
    {
    public:
        virtual ~IAkParametricEQFXParams() = default;
    };

    class AkParametricEQFXParamsOld : public IAkParametricEQFXParams
    {
    public:
        std::vector<AkFilterBand> Bands;
        float OutputLevel = 0;
        bool ProcessLFE = false;

        explicit AkParametricEQFXParamsOld(FWwiseArchive& Ar)
        {
            Bands = Ar.ReadArray<AkFilterBand>(3);
            OutputLevel = Ar.Read<float>();
            ProcessLFE = Ar.Read<uint8_t>() != 0;
        }
    };

    class AkParametricEQFXParams : public IAkParametricEQFXParams
    {
    public:
        std::vector<EQModuleParams> Bands;
        float OutputLevel = 0;
        bool ProcessLFE = false;
        uint32_t SidechainId = 0;
        bool SidechainGlobalScope = false;

        explicit AkParametricEQFXParams(FWwiseArchive& Ar)
        {
            // Faithful quirk: this one uses MathF.Exp, not the usual MathF.Pow(10, x * 0.05f) dB curve.
            OutputLevel = std::exp(Ar.Read<float>() * 0.05f);
            ProcessLFE = Ar.Read<uint8_t>() != 0;
            SidechainId = Ar.Read<uint32_t>();
            SidechainGlobalScope = Ar.Read<uint8_t>() != 0;
            const uint8_t numBands = Ar.Read<uint8_t>();
            Ar.Read<uint32_t>(); // bandEnabledBitfield -- read and dropped, as in C#
            const uint32_t bandDynamicsEnabledBitfield = Ar.Read<uint32_t>();
            auto staticPart = Ar.ReadArray<EQModuleParamsStatic>(numBands);
            // The dynamics block is only present when *any* band enables it; otherwise zeroes are used.
            auto dynamicPart = bandDynamicsEnabledBitfield != 0
                                   ? Ar.ReadArray<EQModuleParamsDynamic>(numBands)
                                   : std::vector<EQModuleParamsDynamic>(numBands, EQModuleParamsDynamic{});
            Bands.resize(numBands);
            for (int i = 0; i < numBands; i++)
            {
                Bands[i].Static = staticPart[i];
                Bands[i].Dynamic = dynamicPart[i];
            }
        }
    };

    class CAkParameterEQFXParams : public IAkPluginParam
    {
    public:
        std::unique_ptr<IAkParametricEQFXParams> Params;

        explicit CAkParameterEQFXParams(FWwiseArchive& Ar)
        {
            if (Ar.Version >= 172)
                Params = std::make_unique<AkParametricEQFXParams>(Ar);
            else
                Params = std::make_unique<AkParametricEQFXParamsOld>(Ar);
        }
    };
}
