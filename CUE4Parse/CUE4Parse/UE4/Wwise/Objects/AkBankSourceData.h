// Ported from CUE4Parse/UE4/Wwise/Objects/AkBankSourceData.cs
#pragma once

#include <cstdint>
#include <memory>

#include "../WwiseArchive.h"
#include "../WwisePlugin.h"
#include "../Enums/EAKBKSourceType.h"
#include "../Enums/EAkPluginType.h"
#include "../Enums/Flags/EBankSourceFlags.h"
#include "../Plugins/IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    using CUE4Parse::UE4::Wwise::AkPlugin;
    using CUE4Parse::UE4::Wwise::WwisePlugin;
    using CUE4Parse::UE4::Wwise::Enums::EAKBKSourceType;
    using CUE4Parse::UE4::Wwise::Enums::EAkPluginType;
    using CUE4Parse::UE4::Wwise::Enums::Flags::EBankSourceFlags;
    using CUE4Parse::UE4::Wwise::Enums::Flags::EBankSourceFlags_v112;
    using CUE4Parse::UE4::Wwise::Plugins::IAkPluginParam;

    // CAkBankMgr::LoadSource
    class AkBankSourceData
    {
    public:
        AkPlugin Plugin;
        EAKBKSourceType SourceType = static_cast<EAKBKSourceType>(0);
        uint32_t DataIndex = 0;
        uint32_t SampleRate = 0;
        uint32_t FormatBits = 0;
        uint32_t SourceId = 0;
        uint32_t FileId = 0;
        uint32_t FileOffset = 0;
        uint32_t InMemoryMediaSize = 0;
        uint32_t CacheId = 0;
        EBankSourceFlags BankSourceFlags = EBankSourceFlags::None;
        bool HasPluginParams = false;
        std::unique_ptr<IAkPluginParam> PluginParams;

        AkBankSourceData() = default;

        explicit AkBankSourceData(FWwiseArchive& Ar)
        {
            Plugin = WwisePlugin::GetPluginId(Ar);
            SourceType = Ar.Version <= 89 ? static_cast<EAKBKSourceType>(Ar.Read<uint32_t>())
                                          : Ar.Read<EAKBKSourceType>();

            if (Ar.Version <= 46)
            {
                if (Ar.Version <= 26)
                {
                    DataIndex = Ar.Read<uint32_t>();
                    SampleRate = Ar.Read<uint32_t>();
                    FormatBits = Ar.Read<uint32_t>();
                }
                else
                {
                    SampleRate = Ar.Read<uint32_t>();
                    FormatBits = Ar.Read<uint32_t>();
                }
            }

            SourceId = Ar.Read<uint32_t>();
            if (Ar.Version <= 26)
            {
                // Do nothing
            }
            else if (Ar.Version <= 88)
            {
                FileId = Ar.Read<uint32_t>();
                // Note the excluded source type differs between the two branches below: PrefetchStreaming
                // here, plain Streaming for versions 89..112.
                if (SourceType != EAKBKSourceType::PrefetchStreaming)
                {
                    FileOffset = Ar.Read<uint32_t>();
                    InMemoryMediaSize = Ar.Read<uint32_t>();
                }
            }
            else if (Ar.Version <= 150)
            {
                if (Ar.Version <= 112)
                {
                    FileId = Ar.Read<uint32_t>();
                    if (SourceType != EAKBKSourceType::Streaming)
                        FileOffset = Ar.Read<uint32_t>();

                    InMemoryMediaSize = Ar.Read<uint32_t>();
                }
                else
                {
                    InMemoryMediaSize = Ar.Read<uint32_t>();
                }
            }
            else
            {
                CacheId = Ar.Read<uint32_t>();
                InMemoryMediaSize = Ar.Read<uint32_t>();
            }

            const uint8_t sourceBits = Ar.Read<uint8_t>();
            // The v112 bit layout is not a subset of the current one -- MapToCurrent re-maps, it does not cast.
            if (Ar.Version <= 112)
                BankSourceFlags = MapToCurrent(static_cast<EBankSourceFlags_v112>(sourceBits));
            else
                BankSourceFlags = static_cast<EBankSourceFlags>(sourceBits);

            bool alwaysParam;
            if (Ar.Version <= 26)
            {
                HasPluginParams = true;
                alwaysParam = true;
            }
            else if (Ar.Version <= 126)
            {
                HasPluginParams = Plugin.Type() == EAkPluginType::Source ||
                                  Plugin.Type() == EAkPluginType::MotionSource;
                alwaysParam = false;
            }
            else
            {
                HasPluginParams = Plugin.Type() == EAkPluginType::Source;
                alwaysParam = false;
            }

            if (HasPluginParams)
                PluginParams = WwisePlugin::TryParsePluginParams(Ar, Plugin, alwaysParam);
        }
    };
}
