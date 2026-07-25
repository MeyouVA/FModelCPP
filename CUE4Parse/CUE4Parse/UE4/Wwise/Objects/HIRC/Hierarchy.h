// Ported from CUE4Parse/UE4/Wwise/Objects/HIRC/Hierarchy.cs
// One entry of the BankHierarchy section: a type byte, a length, and a typed payload.
//
// C# logs through Serilog when the type fails to map, when a parse throws, and (in DEBUG) when the
// reader lands on the wrong byte. This port has no logging layer, so those are dropped -- but the
// recovery behaviour they accompany is kept exactly: rewind and re-read as HierarchyGeneric on a throw,
// and always seek to the declared end.
#pragma once

#include <cstdint>
#include <memory>

#include "../../WwiseArchive.h"
#include "../../Enums/EAKBKHircType.h"
#include "AbstractHierarchy.h"
#include "HierarchyGeneric.h"
#include "Containers/HierarchyActorMixer.h"
#include "Containers/HierarchyAttenuation.h"
#include "Containers/HierarchyBusVariants.h"
#include "Containers/HierarchyDialogueEvent.h"
#include "Containers/HierarchyEvent.h"
#include "Containers/HierarchyEventAction.h"
#include "Containers/HierarchyFxVariants.h"
#include "Containers/HierarchyLayerContainer.h"
#include "Containers/HierarchyModulatorVariants.h"
#include "Containers/HierarchyMusicRandomSequenceContainer.h"
#include "Containers/HierarchyMusicSegment.h"
#include "Containers/HierarchyMusicSwitchContainer.h"
#include "Containers/HierarchyMusicTrack.h"
#include "Containers/HierarchyRandomSequenceContainer.h"
#include "Containers/HierarchySettings.h"
#include "Containers/HierarchySidechainMix.h"
#include "Containers/HierarchySoundSfxVoice.h"
#include "Containers/HierarchySwitchContainer.h"

namespace CUE4Parse::UE4::Wwise::Objects::HIRC
{
    using CUE4Parse::UE4::Wwise::Enums::EAKBKHircType;
    namespace C = CUE4Parse::UE4::Wwise::Objects::HIRC::Containers;

    struct Hierarchy
    {
        EAKBKHircType Type = static_cast<EAKBKHircType>(0);
        uint32_t Length = 0;
        std::unique_ptr<AbstractHierarchy> Data;

        Hierarchy() = default;

        explicit Hierarchy(FWwiseArchive& Ar)
        {
            const uint8_t rawType = Ar.Read<uint8_t>();
            Length = Ar.Read<uint32_t>();

            const int64_t hierarchyStartPosition = Ar.Position;
            const int64_t hierarchyEndPosition = hierarchyStartPosition + Length;

            // Not a cast: the v125 numbering differs from the current one from FxShareSet onwards.
            // Qualified: the arguments are plain integers, so ADL cannot find the Enums overload.
            Type = CUE4Parse::UE4::Wwise::Enums::MapToCurrent(rawType, Ar.Version);
            // C# logs "Failed to map hierarchy type" when Type comes back 0; no logging layer here.

            // Try/Catch is done to allow for extracting audio even if this fails
            // Due to their complexity it's very likely hierarchies will fail to parse if unsupported
            try
            {
                Data = Construct(Ar, Type);
            }
            catch (...)
            {
                Ar.Position = hierarchyStartPosition;
                Data = std::make_unique<HierarchyGeneric>(Ar);
            }

            if (Ar.Position != hierarchyEndPosition)
            {
                Ar.Position = hierarchyEndPosition;
            }
        }

    private:
        static std::unique_ptr<AbstractHierarchy> Construct(FWwiseArchive& Ar, EAKBKHircType type)
        {
            switch (type)
            {
                case EAKBKHircType::State: return std::make_unique<C::HierarchySettings>(Ar);
                case EAKBKHircType::SoundSfxVoice: return std::make_unique<C::HierarchySoundSfxVoice>(Ar);
                case EAKBKHircType::EventAction: return std::make_unique<C::HierarchyEventAction>(Ar);
                case EAKBKHircType::Event: return std::make_unique<C::HierarchyEvent>(Ar);
                case EAKBKHircType::RandomSequenceContainer:
                    return std::make_unique<C::HierarchyRandomSequenceContainer>(Ar);
                case EAKBKHircType::SwitchContainer: return std::make_unique<C::HierarchySwitchContainer>(Ar);
                case EAKBKHircType::ActorMixer: return std::make_unique<C::HierarchyActorMixer>(Ar);
                case EAKBKHircType::AudioBus: return std::make_unique<C::HierarchyAudioBus>(Ar);
                case EAKBKHircType::LayerContainer: return std::make_unique<C::HierarchyLayerContainer>(Ar);
                case EAKBKHircType::MusicSegment: return std::make_unique<C::HierarchyMusicSegment>(Ar);
                case EAKBKHircType::MusicTrack: return std::make_unique<C::HierarchyMusicTrack>(Ar);
                case EAKBKHircType::MusicSwitchContainer:
                    return std::make_unique<C::HierarchyMusicSwitchContainer>(Ar);
                case EAKBKHircType::MusicRandomSequenceContainer:
                    return std::make_unique<C::HierarchyMusicRandomSequenceContainer>(Ar);
                case EAKBKHircType::Attenuation: return std::make_unique<C::HierarchyAttenuation>(Ar);
                case EAKBKHircType::DialogueEvent: return std::make_unique<C::HierarchyDialogueEvent>(Ar);
                case EAKBKHircType::FeedbackBus: return std::make_unique<C::HierarchyFeedbackBus>(Ar);
                case EAKBKHircType::FeedbackNode: return std::make_unique<C::HierarchyFeedbackNode>(Ar);
                case EAKBKHircType::FxShareSet: return std::make_unique<C::HierarchyFxShareSet>(Ar);
                case EAKBKHircType::FxCustom: return std::make_unique<C::HierarchyFxCustom>(Ar);
                case EAKBKHircType::AuxiliaryBus: return std::make_unique<C::HierarchyAuxiliaryBus>(Ar);
                case EAKBKHircType::AudioDevice: return std::make_unique<C::HierarchyAudioDevice>(Ar);
                case EAKBKHircType::LFO: return std::make_unique<C::HierarchyLFO>(Ar);
                case EAKBKHircType::Envelope: return std::make_unique<C::HierarchyEnvelope>(Ar);
                case EAKBKHircType::TimeMod: return std::make_unique<C::HierarchyTimeMod>(Ar);
                case EAKBKHircType::SidechainMix: return std::make_unique<C::HierarchySidechainMix>(Ar);
                default: return std::make_unique<HierarchyGeneric>(Ar);
            }
        }
    };
}
