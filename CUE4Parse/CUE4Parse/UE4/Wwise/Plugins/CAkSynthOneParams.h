// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkSynthOneParams.cs
#pragma once

#include <cstdint>

#include "../WwiseArchive.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    enum class AkSynthOneOperationMode : uint8_t
    {
        Mix = 0x0,
        Ring = 0x1
    };

    enum class AkSynthOneFrequencyMode : uint8_t
    {
        Specify = 0x0,
        MidiNote = 0x1
    };

    enum class AkSynthOneNoiseType : uint8_t
    {
        White = 0x0,
        Pink = 0x1,
        Red = 0x2,
        Purple = 0x3
    };

    enum class AkSynthOneWaveType : uint8_t
    {
        Sine = 0x0,
        Triangle = 0x1,
        Square = 0x2,
        Sawtooth = 0x3
    };

    struct AkSynthOneParams
    {
        // Note the C# field names are crossed: eFreqMode holds the *operation* mode and eOpMode the
        // *frequency* mode. Kept as-is.
        AkSynthOneOperationMode eFreqMode = static_cast<AkSynthOneOperationMode>(0);
        float fBaseFreq = 0;
        AkSynthOneFrequencyMode eOpMode = static_cast<AkSynthOneFrequencyMode>(0);
        float fOutputLevel = 0;
        AkSynthOneNoiseType eNoiseType = static_cast<AkSynthOneNoiseType>(0);
        float fNoiseLevel = 0;
        float fFmAmount = 0;
        bool bOverSampling = false;
        AkSynthOneWaveType eOsc1Waveform = static_cast<AkSynthOneWaveType>(0);
        bool bOsc1Invert = false;
        int32_t iOsc1Transpose = 0;
        float fOsc1Level = 0;
        float fOsc1Pwm = 0;
        AkSynthOneWaveType eOsc2Waveform = static_cast<AkSynthOneWaveType>(0);
        bool bOsc2Invert = false;
        int32_t iOsc2Transpose = 0;
        float fOsc2Level = 0;
        float fOsc2Pwm = 0;

        AkSynthOneParams() = default;

        explicit AkSynthOneParams(FWwiseArchive& Ar)
        {
            eFreqMode = Ar.Read<AkSynthOneOperationMode>();
            fBaseFreq = Ar.Read<float>();
            eOpMode = Ar.Read<AkSynthOneFrequencyMode>();
            fOutputLevel = Ar.Read<float>();
            eNoiseType = Ar.Read<AkSynthOneNoiseType>();
            fNoiseLevel = Ar.Read<float>();
            fFmAmount = Ar.Read<float>();
            bOverSampling = Ar.Read<uint8_t>() != 0;
            eOsc1Waveform = Ar.Read<AkSynthOneWaveType>();
            bOsc1Invert = Ar.Read<uint8_t>() != 0;
            iOsc1Transpose = Ar.Read<int32_t>();
            fOsc1Level = Ar.Read<float>();
            fOsc1Pwm = Ar.Read<float>();
            eOsc2Waveform = Ar.Read<AkSynthOneWaveType>();
            bOsc2Invert = Ar.Read<uint8_t>() != 0;
            iOsc2Transpose = Ar.Read<int32_t>();
            fOsc2Level = Ar.Read<float>();
            fOsc2Pwm = Ar.Read<float>();
        }
    };

    class CAkSynthOneParams : public IAkPluginParam
    {
    public:
        AkSynthOneParams Params;

        explicit CAkSynthOneParams(FWwiseArchive& Ar) : Params(Ar) {}
    };
}
