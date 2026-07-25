// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkGranularSynthParams.cs
#pragma once

#include <cstdint>
#include <vector>

#include "../WwiseArchive.h"
#include "../Objects/AkChannelConfig.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    using CUE4Parse::UE4::Wwise::Objects::AkChannelConfig;

#pragma pack(push, 1)
    struct FGranularModulatorParams
    {
        int32_t ModWaveform;
        uint8_t ModSelect;
        float ModRate;
        float ModPeriod;
        float ModAmount;
    };
#pragma pack(pop)

    struct FGranularValue
    {
        float Value;
        float Mod1Depth;
        float Mod1Quantization;
        float Mod2Depth;
        float Mod2Quantization;
        float Mod3Depth;
        float Mod3Quantization;
        float Mod4Depth;
        float Mod4Quantization;
    };

    class AkGranularSynthParams
    {
    public:
        uint8_t FilterType = 0;
        AkChannelConfig OutputChannelConfig;
        float OutputLevel = 0;
        FGranularValue GrainRate{};
        FGranularValue GrainTime{};
        FGranularValue Offset{};
        FGranularValue Speed{};
        FGranularValue Transpose{};
        FGranularValue Attack{};
        FGranularValue Release{};
        FGranularValue Amplitude{};
        FGranularValue Duration{};
        FGranularValue FilterFreq{};
        FGranularValue FilterQ{};
        FGranularValue Azimuth{};
        FGranularValue Elevation{};
        FGranularValue Spread{};
        FGranularValue MarkerSelect{};
        FGranularValue DurationMultiplier{};
        std::vector<FGranularModulatorParams> Modulators;
        bool MidiMapTranspose = false;
        bool QuantizeToMarkers = false;
        int32_t TransposeRoot = 0;
        int32_t PositioningSelect = 0;
        // These four are declared int in C# but only a byte is read.
        int32_t EnvelopeType = 0;
        int32_t WindowMode = 0;
        int32_t DurationLink = 0;
        int32_t MaxNumGrains = 0;
        int32_t SelectFreqTimeGrain = 0;

        AkGranularSynthParams() = default;

        explicit AkGranularSynthParams(FWwiseArchive& Ar)
        {
            FilterType = Ar.Read<uint8_t>();
            OutputChannelConfig = AkChannelConfig(Ar);
            OutputLevel = Ar.Read<float>();
            GrainRate = Ar.Read<FGranularValue>();
            GrainTime = Ar.Read<FGranularValue>();
            Offset = Ar.Read<FGranularValue>();
            Speed = Ar.Read<FGranularValue>();
            Transpose = Ar.Read<FGranularValue>();
            Attack = Ar.Read<FGranularValue>();
            Release = Ar.Read<FGranularValue>();
            Amplitude = Ar.Read<FGranularValue>();
            Duration = Ar.Read<FGranularValue>();
            FilterFreq = Ar.Read<FGranularValue>();
            FilterQ = Ar.Read<FGranularValue>();
            Azimuth = Ar.Read<FGranularValue>();
            Elevation = Ar.Read<FGranularValue>();
            Spread = Ar.Read<FGranularValue>();
            MarkerSelect = Ar.Read<FGranularValue>();
            DurationMultiplier = Ar.Read<FGranularValue>();
            Modulators = Ar.ReadArray<FGranularModulatorParams>(4);
            MidiMapTranspose = Ar.Read<uint8_t>() != 0;
            QuantizeToMarkers = Ar.Read<uint8_t>() != 0;
            TransposeRoot = Ar.Read<int32_t>();
            PositioningSelect = Ar.Read<int32_t>();
            EnvelopeType = Ar.Read<uint8_t>();
            WindowMode = Ar.Read<uint8_t>();
            DurationLink = Ar.Read<uint8_t>();
            MaxNumGrains = Ar.Read<int32_t>();
            SelectFreqTimeGrain = Ar.Read<uint8_t>();
        }
    };

    class CAkGranularSynthParams : public IAkPluginParam
    {
    public:
        AkGranularSynthParams Params;

        explicit CAkGranularSynthParams(FWwiseArchive& Ar) : Params(Ar) {}
    };
}
