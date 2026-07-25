// Ported from CUE4Parse/UE4/Wwise/Objects/AkChannelConfig.cs
#pragma once

#include <cstdint>

#include "../WwiseArchive.h"
#include "../Enums/EAkChannelConfig.h"
#include "../Enums/EAkChannelConfigType.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    using CUE4Parse::UE4::Wwise::Enums::EAkChannelConfig;
    using CUE4Parse::UE4::Wwise::Enums::EAkChannelConfigType;

    // Can be either this or directly in EAkChannelConfig form
    // https://www.audiokinetic.com/en/public-library/2025.1.4_9062/?source=SDK&id=struct_ak_channel_config.html
    struct AkChannelConfig
    {
        uint8_t NumChannels = 0;
        EAkChannelConfig ConfigTypePacked = static_cast<EAkChannelConfig>(0);
        EAkChannelConfigType ConfigType = static_cast<EAkChannelConfigType>(0);
        uint32_t ChannelMask = 0;

        AkChannelConfig() = default;

        explicit AkChannelConfig(FWwiseArchive& Ar)
        {
            const uint32_t data = Ar.Read<uint32_t>();
            NumChannels = static_cast<uint8_t>(data & 0xFF);
            ConfigTypePacked = static_cast<EAkChannelConfig>(data);
            ConfigType = static_cast<EAkChannelConfigType>((data >> 8) & 0x0F);
            ChannelMask = (data >> 12) & 0xFFFFF;
        }

        // The ushort overload takes a bare mask rather than the packed word: NumChannels stays 0 and the
        // config type is forced to Standard.
        explicit AkChannelConfig(uint16_t channelMask)
        {
            NumChannels = 0;
            ConfigTypePacked = static_cast<EAkChannelConfig>(channelMask);
            ConfigType = EAkChannelConfigType::Standard;
            ChannelMask = channelMask;
        }
    };
}
