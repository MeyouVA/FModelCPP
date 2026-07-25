// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkAsioParams.cs
#pragma once

#include <cstdint>

#include "../WwiseArchive.h"
#include "../Enums/EAkChannelConfig.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    using CUE4Parse::UE4::Wwise::Enums::EAkChannelConfig;

    // The sink and source read identically; C# still declares them separately so the plugin id dispatch
    // stays one-to-one, and that is kept.
    class CAkAsioSinkParams : public IAkPluginParam
    {
    public:
        EAkChannelConfig ChannelConfig;
        int32_t BaseChannel;

        explicit CAkAsioSinkParams(FWwiseArchive& Ar)
            : ChannelConfig(Ar.Read<EAkChannelConfig>()), BaseChannel(Ar.Read<int32_t>()) {}
    };

    class CAkAsioSourceParams : public IAkPluginParam
    {
    public:
        EAkChannelConfig ChannelConfig;
        int32_t BaseChannel;

        explicit CAkAsioSourceParams(FWwiseArchive& Ar)
            : ChannelConfig(Ar.Read<EAkChannelConfig>()), BaseChannel(Ar.Read<int32_t>()) {}
    };
}
