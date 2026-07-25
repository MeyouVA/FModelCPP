// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkReflectFXParams.cs
#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

#include "../WwiseArchive.h"
#include "../Objects/AkChannelConfig.h"
#include "../Objects/AkConversionTable.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    using CUE4Parse::UE4::Wwise::Objects::AkChannelConfig;
    using CUE4Parse::UE4::Wwise::Objects::CAkConversionTable;

#pragma pack(push, 4)
    struct AkFilteredFracDelayLineParams
    {
        float SpeedOfSound;
        float ParamFilterCutoff;
        uint32_t ParamFilterType;
        float PitchThreshold;
        float DistanceThreshold;
        uint32_t ThresholdMode;
    };
#pragma pack(pop)

    struct AkDecorrParams
    {
        float FusingTime = 0;
        float DecorrStrength = 0;
        int32_t DecorrAlgorithmSelect = 0;
        int32_t DecorrStrengthSource = 0;
        uint32_t DecorrFilterMaxReflectionOrder = 0;
        bool StereoDecorrelation = false;
        float DecorrWindowWidth = 0;
        bool DecorrHardwareAcceleration = false;
        uint32_t MaterialFilteringSelect = 0;

        AkDecorrParams() = default;

        explicit AkDecorrParams(FWwiseArchive& Ar)
        {
            FusingTime = Ar.Read<float>();
            DecorrStrength = Ar.Read<float>();
            DecorrAlgorithmSelect = Ar.Read<int32_t>();
            DecorrStrengthSource = Ar.Read<int32_t>();
            DecorrFilterMaxReflectionOrder = Ar.Read<uint32_t>();
            StereoDecorrelation = Ar.Read<uint8_t>() != 0;
            DecorrWindowWidth = Ar.Read<float>();
            DecorrHardwareAcceleration = Ar.Read<uint8_t>() != 0;
            MaterialFilteringSelect = Ar.Version >= 154 ? Ar.Read<uint32_t>() : 0;
        }
    };

    struct AkReflectFXParams
    {
        float CenterPerc = 0;
        float MaxReflections = 0;
        float DryGain = 0;
        float WetGainDB = 0;
        std::vector<CAkConversionTable> m_Curves;
        float MaxDistance = 0;
        float BaseTextureFrequency = 0;
        uint32_t uFadeOutNbFrames = 0;
        AkFilteredFracDelayLineParams delayLineParams{};
        float fPrevDryGain = 0;
        AkChannelConfig OutputChannelConfig;
        float DelayErrorTolerance = 0;
        float DistanceWarping = 0;
        float DiffractionWarping = 0;
        AkDecorrParams DecorrParams;
        float FadeTime = 0;
        bool HardwareProcessing = false;
        float MaxImageSourceDelayTime = 0;

        AkReflectFXParams() = default;

        explicit AkReflectFXParams(FWwiseArchive& Ar)
        {
            delayLineParams.SpeedOfSound = std::max(Ar.Read<float>(), 0.001f);
            CenterPerc = Ar.Read<float>();
            MaxReflections = Ar.Read<float>();
            DryGain = Ar.Read<float>();
            WetGainDB = Ar.Read<float>();
            MaxDistance = Ar.Read<float>();
            BaseTextureFrequency = Ar.Read<float>();
            uFadeOutNbFrames = Ar.Read<uint32_t>();
            delayLineParams.ParamFilterCutoff = Ar.Read<float>();
            delayLineParams.ParamFilterType = Ar.Read<uint32_t>();
            delayLineParams.PitchThreshold = Ar.Read<float>();
            delayLineParams.DistanceThreshold = Ar.Read<float>();
            delayLineParams.ThresholdMode = Ar.Read<uint32_t>();
            if (Ar.Version >= 172)
            {
                DelayErrorTolerance = Ar.Read<float>();
            }
            if (Ar.Version >= 145)
            {
                DistanceWarping = Ar.Read<float>();
                DiffractionWarping = Ar.Read<float>();
            }
            OutputChannelConfig = AkChannelConfig(Ar);
            if (Ar.Version >= 145)
            {
                DecorrParams = AkDecorrParams(Ar);
                FadeTime = Ar.Version >= 154 ? Ar.Read<float>() : 0.0f;
            }
            if (Ar.Version >= 172)
            {
                HardwareProcessing = Ar.Read<uint8_t>() != 0;
                MaxImageSourceDelayTime = Ar.Read<float>();
            }
            // scaling depends on index and wwise version
            // Each curve carries its own destination index, so the array is filled out of order rather
            // than sequentially.
            const uint16_t curvesCount = Ar.Read<uint16_t>();
            m_Curves.resize(curvesCount);
            for (uint16_t i = 0; i < curvesCount; i++)
            {
                const int32_t index = Ar.Read<int32_t>();
                m_Curves[static_cast<size_t>(index)] = CAkConversionTable(Ar, false);
            }
        }
    };

    class CAkReflectFXParams : public IAkPluginParam
    {
    public:
        AkReflectFXParams Params;

        explicit CAkReflectFXParams(FWwiseArchive& Ar) : Params(Ar) {}
    };
}
