// Ported from CUE4Parse/UE4/Wwise/Objects/HIRC/BaseHierarchyBus.cs
#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "../../WwiseArchive.h"
#include "../../Enums/Flags/EAdvSettingsFlags.h"
#include "../AkAuxParams.h"
#include "../AkChannelConfig.h"
#include "../AkDuckInfo.h"
#include "../AkFXParams.h"
#include "../AkFeedbackInfo.h"
#include "../AkPositioningParams.h"
#include "../AkPropBundle.h"
#include "../AkRTPC.h"
#include "../AkStateChunk.h"
#include "AbstractHierarchy.h"

namespace CUE4Parse::UE4::Wwise::Objects::HIRC
{
    using CUE4Parse::UE4::Wwise::Enums::Flags::EAdvSettingsFlags;

    // CAkBus
    // Seven version arms for the settings block: before 90 the advanced-settings flags were individual
    // bools that C# folds back into the flag word, and the channel config came as a bare ushort mask
    // rather than the packed AkChannelConfig word.
    class BaseHierarchyBus : public AbstractHierarchy
    {
    public:
        uint32_t OverrideBusId = 0;
        uint32_t DeviceSharesetId = 0;
        std::vector<AkProp> Props;
        std::optional<AkPositioningParams> PositioningParams;
        std::optional<AkAuxParams> AuxParams;
        std::optional<EAdvSettingsFlags> AdvSettingsParams;
        std::optional<uint16_t> MaxNumInstance;
        AkChannelConfig ChannelConfig;
        std::optional<uint8_t> HdrEnvelopeFlags;
        int32_t RecoveryTime = 0;
        float MaxDuckVolume = 0;
        std::vector<AkDuckInfo> DuckInfo;
        AkFxBus FxBusParams;
        uint8_t OverrideAttachmentParams = 0;
        // Faithful quirk: C# declares FxChunks and never fills it -- only MetadataParams is read.
        std::vector<AkFxChunk> FxChunks;
        std::vector<AkFxChunk> MetadataParams;
        std::vector<AkRtpc> RTPCs;
        std::vector<AkStateGroup> StateGroups;
        std::optional<AkFeedbackInfo> FeedbackInfo;

        BaseHierarchyBus() = default;

        // CAkBus::SetInitialValues
        explicit BaseHierarchyBus(FWwiseArchive& Ar)
        {
            Id = Ar.Read<uint32_t>();
            OverrideBusId = Ar.Read<uint32_t>();
            // Only a bus with no parent carries a device shareset.
            if (Ar.Version > 126 && OverrideBusId == 0)
            {
                DeviceSharesetId = Ar.Read<uint32_t>();
            }

            if (Ar.Version > 56)
            {
                Props = AkPropBundle::ReadSequentialAkProp(Ar);
            }

            if (Ar.Version > 122)
            {
                PositioningParams = AkPositioningParams(Ar);
                AuxParams = AkAuxParams(Ar);
            }

            if (Ar.Version <= 53)
            {
                Ar.Read<float>(); // VolumeMain
                Ar.Read<float>(); // LFEVolumeMain
                Ar.Read<float>(); // PitchMain
                Ar.Read<float>(); // LPFMain

                const bool killNewest = Ar.ReadBool();
                MaxNumInstance = Ar.Read<uint16_t>();
                const bool isMaxNumInstOverrideParent = Ar.ReadBool();

                if (Ar.Version > 48)
                    ChannelConfig = AkChannelConfig(Ar.Read<uint16_t>());

                Ar.Read<uint8_t>();
                Ar.Read<uint8_t>();

                if (Ar.Version > 48)
                    (void) Ar.ReadBool(); // IsEnvironmental

                AdvSettingsParams = EAdvSettingsFlags::None;
                if (killNewest)
                    *AdvSettingsParams |= EAdvSettingsFlags::KillNewest;
                if (isMaxNumInstOverrideParent)
                    *AdvSettingsParams |= EAdvSettingsFlags::IgnoreParentMaxNumInst;
            }
            else if (Ar.Version <= 56)
            {
                Ar.Read<float>(); // VolumeMain
                Ar.Read<float>(); // LFEVolumeMain
                Ar.Read<float>(); // PitchMain
                Ar.Read<float>(); // LPFMain

                const bool killNewest = Ar.ReadBool();
                const bool useVirtualBehavior = Ar.ReadBool();
                MaxNumInstance = Ar.Read<uint16_t>();
                const bool isMaxNumInstOverrideParent = Ar.ReadBool();

                ChannelConfig = AkChannelConfig(Ar.Read<uint16_t>());

                Ar.Read<uint8_t>();
                Ar.Read<uint8_t>();

                (void) Ar.ReadBool(); // bIsEnvBus

                AdvSettingsParams = EAdvSettingsFlags::None;
                if (killNewest)
                    *AdvSettingsParams |= EAdvSettingsFlags::KillNewest;
                if (useVirtualBehavior)
                    *AdvSettingsParams |= EAdvSettingsFlags::UseVirtualBehavior;
                if (isMaxNumInstOverrideParent)
                    *AdvSettingsParams |= EAdvSettingsFlags::IgnoreParentMaxNumInst;
            }
            else if (Ar.Version <= 65)
            {
                const bool killNewest = Ar.ReadBool();
                const bool useVirtualBehavior = Ar.ReadBool();
                MaxNumInstance = Ar.Read<uint16_t>();
                const bool isMaxNumInstOverrideParent = Ar.ReadBool();

                ChannelConfig = AkChannelConfig(Ar.Read<uint16_t>());

                Ar.Read<uint8_t>();
                Ar.Read<uint8_t>();

                (void) Ar.ReadBool(); // bIsEnvBus

                AdvSettingsParams = EAdvSettingsFlags::None;
                if (killNewest)
                    *AdvSettingsParams |= EAdvSettingsFlags::KillNewest;
                if (useVirtualBehavior)
                    *AdvSettingsParams |= EAdvSettingsFlags::UseVirtualBehavior;
                if (isMaxNumInstOverrideParent)
                    *AdvSettingsParams |= EAdvSettingsFlags::IgnoreParentMaxNumInst;
            }
            else if (Ar.Version <= 77)
            {
                // Identical to the <= 65 arm except the trailing bIsEnvBus byte is gone.
                const bool killNewest = Ar.ReadBool();
                const bool useVirtualBehavior = Ar.ReadBool();
                MaxNumInstance = Ar.Read<uint16_t>();
                const bool isMaxNumInstOverrideParent = Ar.ReadBool();

                ChannelConfig = AkChannelConfig(Ar.Read<uint16_t>());

                Ar.Read<uint8_t>();
                Ar.Read<uint8_t>();

                AdvSettingsParams = EAdvSettingsFlags::None;
                if (killNewest)
                    *AdvSettingsParams |= EAdvSettingsFlags::KillNewest;
                if (useVirtualBehavior)
                    *AdvSettingsParams |= EAdvSettingsFlags::UseVirtualBehavior;
                if (isMaxNumInstOverrideParent)
                    *AdvSettingsParams |= EAdvSettingsFlags::IgnoreParentMaxNumInst;
            }
            else if (Ar.Version <= 89)
            {
                Ar.Read<uint8_t>(); // PositioningEnabled
                Ar.Read<uint8_t>(); // PositioningEnablePanner

                const bool killNewest = Ar.ReadBool();
                const bool useVirtualBehavior = Ar.ReadBool();
                MaxNumInstance = Ar.Read<uint16_t>();
                const bool isMaxNumInstOverrideParent = Ar.ReadBool();

                ChannelConfig = AkChannelConfig(Ar.Read<uint16_t>());

                Ar.Read<uint8_t>();
                Ar.Read<uint8_t>();

                (void) Ar.ReadBool(); // isHdrBus
                (void) Ar.ReadBool(); // hdrReleaseModeExponential

                AdvSettingsParams = EAdvSettingsFlags::None;
                if (killNewest)
                    *AdvSettingsParams |= EAdvSettingsFlags::KillNewest;
                if (useVirtualBehavior)
                    *AdvSettingsParams |= EAdvSettingsFlags::UseVirtualBehavior;
                if (isMaxNumInstOverrideParent)
                    *AdvSettingsParams |= EAdvSettingsFlags::IgnoreParentMaxNumInst;

                // C# leaves HdrEnvelopeFlags unset here, with the two isHdrBus/hdrReleaseModeExponential
                // assignments commented out. Kept unset.
            }
            else if (Ar.Version <= 122)
            {
                Ar.Read<uint8_t>(); // the only difference from the modern arm: one leading byte
                AdvSettingsParams = Ar.Read<EAdvSettingsFlags>();
                MaxNumInstance = Ar.Read<uint16_t>();
                ChannelConfig = AkChannelConfig(Ar);
                HdrEnvelopeFlags = Ar.Read<uint8_t>();
            }
            else
            {
                AdvSettingsParams = Ar.Read<EAdvSettingsFlags>();
                MaxNumInstance = Ar.Read<uint16_t>();
                ChannelConfig = AkChannelConfig(Ar);
                HdrEnvelopeFlags = Ar.Read<uint8_t>();
            }

            if (Ar.Version <= 52)
            {
                Ar.Read<uint32_t>(); // stateGroupId
            }

            RecoveryTime = Ar.Read<int32_t>();

            if (Ar.Version > 38)
            {
                MaxDuckVolume = Ar.Read<float>();
            }

            if (Ar.Version <= 52)
            {
                Ar.Read<uint32_t>(); // stateSyncType
            }

            const int duckCount = static_cast<int>(Ar.Read<uint32_t>());
            DuckInfo = Ar.ReadArrayWith(duckCount, [&Ar] { return AkDuckInfo(Ar); });

            FxBusParams = AkFxBus(Ar);

            if (Ar.Version > 89 && Ar.Version <= 145)
            {
                OverrideAttachmentParams = Ar.Read<uint8_t>();
            }

            if (Ar.Version > 136)
            {
                const int metaCount = Ar.Read<uint8_t>();
                MetadataParams = Ar.ReadArrayWith(metaCount, [&Ar] { return AkFxChunk(Ar); });
            }

            RTPCs = AkRtpc::ReadArray(Ar);

            // As in BaseHierarchy, C# splits <= 52 and <= 122 but both read the same chunk today.
            if (Ar.Version <= 122)
            {
                StateGroups = AkStateChunk(Ar).Groups;
            }
            else
            {
                StateGroups = AkStateAwareChunk(Ar).Groups;
            }

            if (Ar.Version <= 126 && Ar.HasFeedback)
            {
                FeedbackInfo = AkFeedbackInfo(Ar);
            }
        }
    };
}
