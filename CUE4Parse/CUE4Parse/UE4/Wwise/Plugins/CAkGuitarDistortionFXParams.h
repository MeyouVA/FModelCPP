// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkGuitarDistortionFXParams.cs
// AkFilterBand is declared in this C# file but lives in CAkParameterEQFXParams.h here (that header needs
// it first). Same namespace, so nothing else changes.
#pragma once

#include <cstdint>
#include <vector>

#include "../WwiseArchive.h"
#include "CAkParameterEQFXParams.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    enum class AkDistortionType : uint32_t
    {
        None = 0x0,
        Overdrive = 0x1,
        Heavy = 0x2,
        Fuzz = 0x3,
        Clip = 0x4
    };

#pragma pack(push, 4)
    struct AkDistortionParams
    {
        AkDistortionType DistortionType;
        float Drive;
        float Tone;
        float Rectification;
    };
#pragma pack(pop)

    struct AkGuitarDistortionParams
    {
        std::vector<AkFilterBand> PreEQ;
        std::vector<AkFilterBand> PostEQ;
        AkDistortionParams Distortion{};
        float OutputLevel = 0;
        float WetDryMix = 0;

        AkGuitarDistortionParams() = default;

        explicit AkGuitarDistortionParams(FWwiseArchive& Ar)
        {
            PreEQ = Ar.ReadArray<AkFilterBand>(3);
            PostEQ = Ar.ReadArray<AkFilterBand>(3);
            Distortion = Ar.Read<AkDistortionParams>();
            OutputLevel = DbToLinear(Ar.Read<float>());
            WetDryMix = Ar.Read<float>();
        }
    };

    class CAkGuitarDistortionFXParams : public IAkPluginParam
    {
    public:
        AkGuitarDistortionParams Params;

        explicit CAkGuitarDistortionFXParams(FWwiseArchive& Ar) : Params(Ar) {}
    };
}
