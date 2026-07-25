// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkSystemOutputParams.cs
#pragma once

#include <cstdint>

#include "../WwiseArchive.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    enum class AkAudioObjectDestination : int32_t
    {
        Default = 0x0,
        MainMix = 0x1,
        Passthrough = 0x2,
        SystemAudioObject = 0x3
    };

    class CAkSystemOutputParams : public IAkPluginParam
    {
    public:
        AkAudioObjectDestination Destination;

        explicit CAkSystemOutputParams(FWwiseArchive& Ar) : Destination(Ar.Read<AkAudioObjectDestination>()) {}
    };
}
