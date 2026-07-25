// Ported from CUE4Parse/UE4/Wwise/Plugins/MasteringSuite/CMasteringSuiteFXParams.cs
// The Sce* type names are Sony's -- this is the PS4/PS5 mastering suite as Wwise wraps it.
#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "../../WwiseArchive.h"
#include "../IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins::MasteringSuite
{
#pragma pack(push, 1)
    struct SceAudioOut2MasteringParamEqFilterParams
    {
        uint32_t m_eqBandsFilterMode;
        float frequency;
        float gain;
        float resonance;
    };
#pragma pack(pop)

#pragma pack(push, 4)
    struct SceAudioOut2MasteringCompressorBandParams
    {
        float threshold;
        float ratio;
        float attack;
        float release;
        float makeupGain;
        float knee;
    };
#pragma pack(pop)

    struct SceAudioOut2MasteringParamEqParamsV2
    {
        uint32_t numBands = 0;
        std::array<bool, 6> m_eqBandsBypassFlags{};
        std::vector<SceAudioOut2MasteringParamEqFilterParams> filterParams;

        SceAudioOut2MasteringParamEqParamsV2() = default;

        // The bypass flags are always six, regardless of numBands.
        explicit SceAudioOut2MasteringParamEqParamsV2(FWwiseArchive& Ar)
        {
            numBands = Ar.Read<uint32_t>();
            for (auto& b : m_eqBandsBypassFlags) b = Ar.Read<uint8_t>() != 0;
            filterParams = Ar.ReadArray<SceAudioOut2MasteringParamEqFilterParams>(6);
        }
    };

    struct SceAudioOut2MasteringCompressorParamsV2
    {
        uint32_t numBands = 0;
        uint32_t linkMode = 0;
        float linkStrength = 0;
        bool linkStereoPairs = false;
        std::array<bool, 4> bandsBypassFlags{};
        std::vector<float> crossoverFrequencies;
        std::vector<SceAudioOut2MasteringCompressorBandParams> bandParams;

        SceAudioOut2MasteringCompressorParamsV2() = default;

        explicit SceAudioOut2MasteringCompressorParamsV2(FWwiseArchive& Ar)
        {
            numBands = Ar.Read<uint32_t>();
            linkMode = Ar.Read<uint32_t>();
            linkStrength = Ar.Read<float>();
            linkStereoPairs = Ar.Read<uint8_t>() != 0;
            for (auto& b : bandsBypassFlags) b = Ar.Read<uint8_t>() != 0;
            crossoverFrequencies = Ar.ReadArray<float>(3);
            bandParams = Ar.ReadArray<SceAudioOut2MasteringCompressorBandParams>(4);
        }
    };

    struct SceAudioOut2MasteringLimiterParamsV2
    {
        uint32_t mode = 0;
        float threshold = 0;
        float attack = 0;
        float release = 0;
        float outputGain = 0;
        // C#'s field name has the stray capital: linkMOde. Kept.
        bool linkMOde = false;

        SceAudioOut2MasteringLimiterParamsV2() = default;

        explicit SceAudioOut2MasteringLimiterParamsV2(FWwiseArchive& Ar)
        {
            mode = Ar.Read<uint32_t>();
            threshold = Ar.Read<float>();
            attack = Ar.Read<float>();
            release = Ar.Read<float>();
            outputGain = Ar.Read<float>();
            linkMOde = Ar.Read<uint8_t>() != 0;
        }
    };

    struct MasteringSuiteFXParams
    {
        std::array<bool, 4> moduleBypassFlags{};
        SceAudioOut2MasteringParamEqParamsV2 paramEqParams;
        SceAudioOut2MasteringCompressorParamsV2 compressorParams;
        std::vector<float> masterVolumeParams;
        SceAudioOut2MasteringLimiterParamsV2 limiterParams;

        MasteringSuiteFXParams() = default;

        explicit MasteringSuiteFXParams(FWwiseArchive& Ar)
        {
            for (auto& b : moduleBypassFlags) b = Ar.Read<uint8_t>() != 0;
            paramEqParams = SceAudioOut2MasteringParamEqParamsV2(Ar);
            compressorParams = SceAudioOut2MasteringCompressorParamsV2(Ar);
            masterVolumeParams = Ar.ReadArray<float>(12);
            limiterParams = SceAudioOut2MasteringLimiterParamsV2(Ar);
        }
    };

    class CMasteringSuiteFXParams : public IAkPluginParam
    {
    public:
        MasteringSuiteFXParams Params;

        explicit CMasteringSuiteFXParams(FWwiseArchive& Ar) : Params(Ar) {}
    };
}
