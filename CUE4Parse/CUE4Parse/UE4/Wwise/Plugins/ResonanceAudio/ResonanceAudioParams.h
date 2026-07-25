// Ported from CUE4Parse/UE4/Wwise/Plugins/ResonanceAudio/ResonanceAudioParams.cs
#pragma once

#include <cstdint>

#include "../../WwiseArchive.h"
#include "../IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins::ResonanceAudio
{
    class ResonanceAudioParams : public IAkPluginParam
    {
    public:
        bool Bypass;

        explicit ResonanceAudioParams(FWwiseArchive& Ar) : Bypass(Ar.Read<uint8_t>() != 0) {}
    };
}
