// Ported from CUE4Parse/UE4/Wwise/Objects/GlobalSettings.cs
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "../WwiseArchive.h"
#include "../Enums/EAkFilterBehavior.h"
#include "AkAcousticTexture.h"
#include "AkDiffuseReverberator.h"
#include "AkRTPCRamping.h"
#include "AkStateGroupInfo.h"
#include "AkSwitchGroup.h"
#include "ICAkIndexable.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    using CUE4Parse::UE4::Wwise::Enums::EAkFilterBehavior;

    class GlobalSettings
    {
    public:
        EAkFilterBehavior FilterBehavior = static_cast<EAkFilterBehavior>(0);
        float VolumeThreshold = 0;
        uint16_t MaxNumVoicesLimitInternal = 0;
        uint16_t MaxNumDangerousVirtVoicesLimitInternal = 0;
        float HSFEmphasis = 0;
        std::vector<AkStateGroupInfo> StateGroups;
        std::vector<AkSwitchGroup> SwitchGroups;
        std::vector<AkRTPCRamping> RTPCRampingParams;
        // C#'s List<ICAkIndexable> mixes AkAcousticTexture, AkAcousticTexture_v122 and AkDiffuseReverberator,
        // so this has to be a polymorphic collection rather than a vector of one concrete type.
        std::vector<std::unique_ptr<ICAkIndexable>> VirtualAcoustics;

        GlobalSettings() = default;

        // CAkBankMgr::ProcessGlobalSettingsChunk
        explicit GlobalSettings(FWwiseArchive& Ar)
        {
            // AkFilterBehavior::SetInternal
            if (Ar.Version > 140)
                FilterBehavior = Ar.Read<EAkFilterBehavior>();

            // AK::SoundEngine::SetVolumeThresholdInternal
            VolumeThreshold = Ar.Read<float>();

            // AK::SoundEngine::SetMaxNumVoicesLimitInternal
            if (Ar.Version > 53)
                MaxNumVoicesLimitInternal = Ar.Read<uint16_t>();

            // AK::SoundEngine::SetMaxNumDangerousVirtVoicesLimitInternal
            if (Ar.Version > 126)
                MaxNumDangerousVirtVoicesLimitInternal = Ar.Read<uint16_t>();

            // AK::SoundEngine::SetHSFEmphasis
            if (Ar.Version > 154)
                HSFEmphasis = Ar.Read<float>();

            // CAkStateMgr::AddStateGroup
            StateGroups = Ar.ReadArrayWith([&Ar] { return AkStateGroupInfo(Ar); });
            SwitchGroups = Ar.ReadArrayWith([&Ar] { return AkSwitchGroup(Ar); });
            if (Ar.Version <= 38)
                return;
            RTPCRampingParams = Ar.ReadArrayWith([&Ar] { return AkRTPCRamping(Ar); });

            // CAkVirtualAcousticsMgr::AddAcousticTexture
            if (Ar.Version <= 118)
            {
                // nothing
            }
            else if (Ar.Version <= 122)
            {
                const int count = Ar.Read<int32_t>();
                for (int i = 0; i < count; i++)
                    VirtualAcoustics.push_back(std::make_unique<AkAcousticTexture_v122>(Ar));
            }
            else
            {
                const int count = Ar.Read<int32_t>();
                for (int i = 0; i < count; i++)
                    VirtualAcoustics.push_back(std::make_unique<AkAcousticTexture>(Ar));
            }

            if (Ar.Version > 118 && Ar.Version <= 122)
            {
                const int count = Ar.Read<int32_t>();
                for (int i = 0; i < count; i++)
                    VirtualAcoustics.push_back(std::make_unique<AkDiffuseReverberator>(Ar));
            }
        }
    };
}
