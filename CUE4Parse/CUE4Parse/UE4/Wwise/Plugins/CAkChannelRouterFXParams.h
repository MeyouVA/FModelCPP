// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkChannelRouterFXParams.cs
#pragma once

#include "../WwiseArchive.h"
#include "../Enums/EAkChannelConfig.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    using CUE4Parse::UE4::Wwise::Enums::EAkChannelConfig;

    class CAkChannelRouterFXParams : public IAkPluginParam
    {
    public:
        EAkChannelConfig BusChannelConfig;

        explicit CAkChannelRouterFXParams(FWwiseArchive& Ar) : BusChannelConfig(Ar.Read<EAkChannelConfig>()) {}
    };

    class CAkChannelRouterMetaParams : public IAkPluginParam
    {
    public:
        EAkChannelConfig BusChannelConfig;

        explicit CAkChannelRouterMetaParams(FWwiseArchive& Ar) : BusChannelConfig(Ar.Read<EAkChannelConfig>()) {}
    };
}
