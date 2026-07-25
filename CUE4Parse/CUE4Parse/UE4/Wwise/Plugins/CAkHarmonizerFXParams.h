// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkHarmonizerFXParams.cs
#pragma once

#include <cstdint>
#include <vector>

#include "../WwiseArchive.h"
#include "CAkPitchShifterFXParams.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    struct AkHarmonizerFXParams
    {
        std::vector<AkPitchVoiceParams> Voice;
        AkInputType InputType = static_cast<AkInputType>(0);
        float DryLevel = 0;
        float WetLevel = 0;
        uint32_t WindowSize = 0;
        bool ProcessLFE = false;
        bool SyncDry = false;

        AkHarmonizerFXParams() = default;

        explicit AkHarmonizerFXParams(FWwiseArchive& Ar)
        {
            // Two voices, each read through the full AkPitchVoiceParams reader.
            Voice = Ar.ReadArrayWith(2, [&Ar] { return AkPitchVoiceParams(Ar); });

            InputType = Ar.Read<AkInputType>();
            DryLevel = DbToLinear(Ar.Read<float>());
            WetLevel = DbToLinear(Ar.Read<float>());
            WindowSize = Ar.Read<uint32_t>();
            ProcessLFE = Ar.Read<uint8_t>() != 0;
            SyncDry = Ar.Read<uint8_t>() != 0;
        }
    };

    class CAkHarmonizerFXParams : public IAkPluginParam
    {
    public:
        AkHarmonizerFXParams Params;

        explicit CAkHarmonizerFXParams(FWwiseArchive& Ar) : Params(Ar) {}
    };
}
