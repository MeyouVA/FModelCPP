// Ported from CUE4Parse/UE4/Wwise/Objects/HIRC/BaseHierarchyFx.cs
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "../../WwiseArchive.h"
#include "../../WwisePlugin.h"
#include "../../Enums/EAkRtpcAccum.h"
#include "../../Plugins/IAkPluginParam.h"
#include "../AkMediaMap.h"
#include "../AkRTPC.h"
#include "../AkStateChunk.h"
#include "AbstractHierarchy.h"

namespace CUE4Parse::UE4::Wwise::Objects::HIRC
{
    using CUE4Parse::UE4::Wwise::AkPlugin;
    using CUE4Parse::UE4::Wwise::WwisePlugin;
    using CUE4Parse::UE4::Wwise::Enums::EAkRtpcAccum;
    using CUE4Parse::UE4::Wwise::Plugins::IAkPluginParam;

    // CAkFxBase
    class BaseHierarchyFx : public AbstractHierarchy
    {
    public:
        struct RtpcInit
        {
            int ParamId = 0;
            float InitValue = 0;

            RtpcInit() = default;

            explicit RtpcInit(FWwiseArchive& Ar)
            {
                ParamId = Ar.Read7BitEncodedIntBE();
                InitValue = Ar.Read<float>();
            }
        };

        struct PluginPropertyValue
        {
            int PropertyId = 0;
            EAkRtpcAccum RtpcAccum = static_cast<EAkRtpcAccum>(0);
            float Value = 0;

            PluginPropertyValue() = default;

            explicit PluginPropertyValue(FWwiseArchive& Ar)
            {
                PropertyId = Ar.Read7BitEncodedIntBE();
                RtpcAccum = Ar.Read<EAkRtpcAccum>();
                Value = Ar.Read<float>();
            }
        };

        std::vector<AkMediaMap> MediaList;
        std::vector<AkRtpc> RTPCs;
        std::vector<AkStateGroup> StateGroups;
        std::vector<RtpcInit> RtpcInitList;
        std::vector<PluginPropertyValue> PluginPropertyValues;
        AkPlugin Plugin;
        std::unique_ptr<IAkPluginParam> PluginParams;

        // CAkFxBase::SetInitialValues
        explicit BaseHierarchyFx(FWwiseArchive& Ar)
        {
            Id = Ar.Read<uint32_t>();
            Plugin = WwisePlugin::GetPluginId(Ar);
            PluginParams = WwisePlugin::TryParsePluginParams(Ar, Plugin);

            const int mediaCount = Ar.Read<uint8_t>();
            MediaList = Ar.ReadArrayWith(mediaCount, [&Ar] { return AkMediaMap(Ar); });
            RTPCs = AkRtpc::ReadArray(Ar);

            if (Ar.Version <= 89)
            {
                // nothing
            }
            else if (Ar.Version <= 126)
            {
                if (Ar.Version > 122)
                {
                    // Unused bytes
                    Ar.Read<uint8_t>();
                    Ar.Read<uint8_t>();
                }

                const int initCount = Ar.Read<uint16_t>();
                RtpcInitList = Ar.ReadArrayWith(initCount, [&Ar] { return RtpcInit(Ar); });
            }
            else
            {
                StateGroups = AkStateAwareChunk(Ar).Groups;
                const int propCount = Ar.Read<uint16_t>();
                PluginPropertyValues = Ar.ReadArrayWith(propCount, [&Ar] { return PluginPropertyValue(Ar); });
            }
        }
    };
}
