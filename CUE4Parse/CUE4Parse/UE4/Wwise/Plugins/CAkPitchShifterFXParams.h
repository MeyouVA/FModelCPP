// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkPitchShifterFXParams.cs
#pragma once

#include <cmath>
#include <cstdint>

#include "../WwiseArchive.h"
#include "CAkParameterEQFXParams.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    enum class AkInputType : uint32_t
    {
        AsInput = 0x0,
        Center = 0x1,
        Stereo = 0x2,
        ThreePointZero = 0x3,
        FourPointZero = 0x4,
        FivePointZero = 0x5
    };

    // 0.000833333354f is 1/1200 -- the wire value is in cents, so this is 2^(cents/1200).
#pragma pack(push, 1)
    struct AkPitchVoiceParams
    {
        AkFilterParams Filter;
        float fPitchFactor;
        float fGain;
        bool bEnable;

        AkPitchVoiceParams() = default;

        // Note the read order is not the declaration order: the enable flag comes first on the wire.
        explicit AkPitchVoiceParams(FWwiseArchive& Ar)
        {
            bEnable = Ar.Read<uint8_t>() != 0;
            fPitchFactor = std::pow(2.0f, Ar.Read<float>() * 0.000833333354f);
            fGain = DbToLinear(Ar.Read<float>());
            Filter = Ar.Read<AkFilterParams>();
        }
    };
#pragma pack(pop)

    // to-do recheck cause BN failed to decompile correctly
    struct AkPitchShifterFXParams
    {
        AkPitchVoiceParams Voice{};
        AkInputType eInputType = static_cast<AkInputType>(0);
        float fDryLevel = 0;
        float fWetLevel = 0;
        float fDelayTime = 0;
        bool bProcessLFE = false;
        bool bSyncDry = false;

        AkPitchShifterFXParams() = default;

        // Unlike AkPitchVoiceParams(Ar) this fills the voice inline and reads only two of its fields.
        explicit AkPitchShifterFXParams(FWwiseArchive& Ar)
        {
            eInputType = Ar.Read<AkInputType>();
            fDryLevel = DbToLinear(Ar.Read<float>());
            fWetLevel = DbToLinear(Ar.Read<float>());
            fDelayTime = Ar.Read<float>();
            bProcessLFE = Ar.Read<uint8_t>() != 0;
            bSyncDry = Ar.Read<uint8_t>() != 0;
            Voice.fPitchFactor = std::pow(2.0f, Ar.Read<float>() * 0.000833333354f);
            Voice.Filter = Ar.Read<AkFilterParams>();
        }
    };

    class CAkPitchShifterFXParams : public IAkPluginParam
    {
    public:
        AkPitchShifterFXParams Params;

        explicit CAkPitchShifterFXParams(FWwiseArchive& Ar) : Params(Ar) {}
    };
}
