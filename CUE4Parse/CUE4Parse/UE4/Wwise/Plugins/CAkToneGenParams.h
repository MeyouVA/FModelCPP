// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkToneGenParams.cs
#pragma once

#include <cstdint>

#include "../WwiseArchive.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    enum class AkToneGenSweep : uint32_t
    {
        LIN = 0x0,
        LOG = 0x1
    };

    enum class AkToneGenType : uint32_t
    {
        SINE = 0x0,
        TRIANGLE = 0x1,
        SQUARE = 0x2,
        SAWTOOTH = 0x3,
        WHITENOISE = 0x4,
        PINKNOISE = 0x5
    };

    enum class AkToneGenMode : uint32_t
    {
        FIX = 0x0,
        ENV = 0x1
    };

#pragma pack(push, 1)
    struct AkToneGenStaticParams
    {
        float fStartFreqRandMin;
        float fStartFreqRandMax;
        uint8_t bFreqSweep;
        AkToneGenSweep eGenSweep;
        float fStopFreqRandMin;
        float fStopFreqRandMax;
        AkToneGenType eGenType;
        AkToneGenMode eGenMode;
        float fFixDur;
        float fAttackDur;
        float fDecayDur;
        float fSustainDur;
        float fSustainVal;
        float fReleaseDur;
        // maybe enum fmt_ch -> enum Audio::EAudioMixerChannel::Type
        uint32_t uChannelMask;
    };

    struct AkToneGenParams
    {
        float fGain;
        float fStartFreq;
        float fStopFreq;
        AkToneGenStaticParams staticParams;
    };
#pragma pack(pop)

    class CAkToneGenParams : public IAkPluginParam
    {
    public:
        AkToneGenParams Params;

        // Blitted whole -- note this means fGain is *not* dB-converted, unlike most gains in this tree.
        explicit CAkToneGenParams(FWwiseArchive& Ar) : Params(Ar.Read<AkToneGenParams>()) {}
    };
}
