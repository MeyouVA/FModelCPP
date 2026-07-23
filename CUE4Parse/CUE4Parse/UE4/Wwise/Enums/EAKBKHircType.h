// Ported from CUE4Parse/UE4/Wwise/Enums/EAKBKHircType.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums
{
    // Versions > 125, default
    enum class EAKBKHircType : uint8_t
    {
        State                        = 0x01, // Removed
        SoundSfxVoice,
        EventAction,
        Event,
        RandomSequenceContainer,
        SwitchContainer,
        ActorMixer,
        AudioBus,
        LayerContainer,
        MusicSegment,
        MusicTrack,
        MusicSwitchContainer,
        MusicRandomSequenceContainer,
        Attenuation,
        DialogueEvent,
        FxShareSet,
        FxCustom,
        AuxiliaryBus,
        LFO,
        Envelope,
        AudioDevice,
        TimeMod,
        SidechainMix, // >= 168
        // Legacy hierarchies
        FeedbackBus                  = 0x80,
        FeedbackNode                 = 0x81,
    };

    // C#'s EAKBKHircType.ToString(). Needed because an extension method below returns the member
    // name as a string. Returns nullptr for a value that is not a declared member
    // (C# would render the number instead).
    inline const char* NameOf(EAKBKHircType value)
    {
        switch (value)
        {
        case EAKBKHircType::State: return "State";
        case EAKBKHircType::SoundSfxVoice: return "SoundSfxVoice";
        case EAKBKHircType::EventAction: return "EventAction";
        case EAKBKHircType::Event: return "Event";
        case EAKBKHircType::RandomSequenceContainer: return "RandomSequenceContainer";
        case EAKBKHircType::SwitchContainer: return "SwitchContainer";
        case EAKBKHircType::ActorMixer: return "ActorMixer";
        case EAKBKHircType::AudioBus: return "AudioBus";
        case EAKBKHircType::LayerContainer: return "LayerContainer";
        case EAKBKHircType::MusicSegment: return "MusicSegment";
        case EAKBKHircType::MusicTrack: return "MusicTrack";
        case EAKBKHircType::MusicSwitchContainer: return "MusicSwitchContainer";
        case EAKBKHircType::MusicRandomSequenceContainer: return "MusicRandomSequenceContainer";
        case EAKBKHircType::Attenuation: return "Attenuation";
        case EAKBKHircType::DialogueEvent: return "DialogueEvent";
        case EAKBKHircType::FxShareSet: return "FxShareSet";
        case EAKBKHircType::FxCustom: return "FxCustom";
        case EAKBKHircType::AuxiliaryBus: return "AuxiliaryBus";
        case EAKBKHircType::LFO: return "LFO";
        case EAKBKHircType::Envelope: return "Envelope";
        case EAKBKHircType::AudioDevice: return "AudioDevice";
        case EAKBKHircType::TimeMod: return "TimeMod";
        case EAKBKHircType::SidechainMix: return "SidechainMix";
        case EAKBKHircType::FeedbackBus: return "FeedbackBus";
        case EAKBKHircType::FeedbackNode: return "FeedbackNode";
        }
        return nullptr;
    }

    // Versions <= 125
    enum class EAKBKHircType_v125 : uint8_t
    {
        Settings                     = 0x01,
        SoundSfxVoice,
        EventAction,
        Event,
        RandomSequenceContainer,
        SwitchContainer,
        ActorMixer,
        AudioBus,
        LayerContainer,
        MusicSegment,
        MusicTrack,
        MusicSwitchContainer,
        MusicRandomSequenceContainer,
        Attenuation,
        DialogueEvent,
        FeedbackBus,
        FeedbackNode,
        FxShareSet,
        FxCustom,
        AuxiliaryBus,
        LFO,
        Envelope,
        AudioDevice,
    };

    // C#'s EAKBKHircType_v125.ToString(). Needed because an extension method below returns the member
    // name as a string. Returns nullptr for a value that is not a declared member
    // (C# would render the number instead).
    inline const char* NameOf(EAKBKHircType_v125 value)
    {
        switch (value)
        {
        case EAKBKHircType_v125::Settings: return "Settings";
        case EAKBKHircType_v125::SoundSfxVoice: return "SoundSfxVoice";
        case EAKBKHircType_v125::EventAction: return "EventAction";
        case EAKBKHircType_v125::Event: return "Event";
        case EAKBKHircType_v125::RandomSequenceContainer: return "RandomSequenceContainer";
        case EAKBKHircType_v125::SwitchContainer: return "SwitchContainer";
        case EAKBKHircType_v125::ActorMixer: return "ActorMixer";
        case EAKBKHircType_v125::AudioBus: return "AudioBus";
        case EAKBKHircType_v125::LayerContainer: return "LayerContainer";
        case EAKBKHircType_v125::MusicSegment: return "MusicSegment";
        case EAKBKHircType_v125::MusicTrack: return "MusicTrack";
        case EAKBKHircType_v125::MusicSwitchContainer: return "MusicSwitchContainer";
        case EAKBKHircType_v125::MusicRandomSequenceContainer: return "MusicRandomSequenceContainer";
        case EAKBKHircType_v125::Attenuation: return "Attenuation";
        case EAKBKHircType_v125::DialogueEvent: return "DialogueEvent";
        case EAKBKHircType_v125::FeedbackBus: return "FeedbackBus";
        case EAKBKHircType_v125::FeedbackNode: return "FeedbackNode";
        case EAKBKHircType_v125::FxShareSet: return "FxShareSet";
        case EAKBKHircType_v125::FxCustom: return "FxCustom";
        case EAKBKHircType_v125::AuxiliaryBus: return "AuxiliaryBus";
        case EAKBKHircType_v125::LFO: return "LFO";
        case EAKBKHircType_v125::Envelope: return "Envelope";
        case EAKBKHircType_v125::AudioDevice: return "AudioDevice";
        }
        return nullptr;
    }

    // C#'s EHierarchyObjectTypeExtensions. Hand-written (the generator skips extension classes); C#'s
    // extension methods become free functions taking the value as the first argument.
    //
    // The two enums are NOT a straight cast of one another: v125 lists FeedbackBus/FeedbackNode inline at
    // 0x10/0x11, while the current enum moved them out to 0x80/0x81 and reused 0x10 for FxShareSet. So every
    // member from FxShareSet onwards is shifted by two between the two layouts, and 0x01 is Settings in v125
    // but State in the current one. That is the whole reason these helpers exist.

    // C#'s EAKBKHircType.ToV125String(). Private in C#; kept here because ToVersionString needs it.
    inline const char* ToV125String(EAKBKHircType type)
    {
        switch (type)
        {
        case EAKBKHircType::State: return NameOf(EAKBKHircType_v125::Settings);
        case EAKBKHircType::SoundSfxVoice: return NameOf(EAKBKHircType_v125::SoundSfxVoice);
        case EAKBKHircType::EventAction: return NameOf(EAKBKHircType_v125::EventAction);
        case EAKBKHircType::Event: return NameOf(EAKBKHircType_v125::Event);
        case EAKBKHircType::RandomSequenceContainer: return NameOf(EAKBKHircType_v125::RandomSequenceContainer);
        case EAKBKHircType::SwitchContainer: return NameOf(EAKBKHircType_v125::SwitchContainer);
        case EAKBKHircType::ActorMixer: return NameOf(EAKBKHircType_v125::ActorMixer);
        case EAKBKHircType::AudioBus: return NameOf(EAKBKHircType_v125::AudioBus);
        case EAKBKHircType::LayerContainer: return NameOf(EAKBKHircType_v125::LayerContainer);
        case EAKBKHircType::MusicSegment: return NameOf(EAKBKHircType_v125::MusicSegment);
        case EAKBKHircType::MusicTrack: return NameOf(EAKBKHircType_v125::MusicTrack);
        case EAKBKHircType::MusicSwitchContainer: return NameOf(EAKBKHircType_v125::MusicSwitchContainer);
        case EAKBKHircType::MusicRandomSequenceContainer: return NameOf(EAKBKHircType_v125::MusicRandomSequenceContainer);
        case EAKBKHircType::Attenuation: return NameOf(EAKBKHircType_v125::Attenuation);
        case EAKBKHircType::DialogueEvent: return NameOf(EAKBKHircType_v125::DialogueEvent);
        case EAKBKHircType::FeedbackBus: return NameOf(EAKBKHircType_v125::FeedbackBus);
        case EAKBKHircType::FeedbackNode: return NameOf(EAKBKHircType_v125::FeedbackNode);
        case EAKBKHircType::FxShareSet: return NameOf(EAKBKHircType_v125::FxShareSet);
        case EAKBKHircType::FxCustom: return NameOf(EAKBKHircType_v125::FxCustom);
        case EAKBKHircType::AuxiliaryBus: return NameOf(EAKBKHircType_v125::AuxiliaryBus);
        case EAKBKHircType::LFO: return NameOf(EAKBKHircType_v125::LFO);
        case EAKBKHircType::Envelope: return NameOf(EAKBKHircType_v125::Envelope);
        case EAKBKHircType::AudioDevice: return NameOf(EAKBKHircType_v125::AudioDevice);
        default: return NameOf(type);
        }
    }

    // C# returns a string; here a `const char*`, which is null for a value that is not a declared member
    // (C#'s ToString() would render the number). Callers that need a string must handle null.
    inline const char* ToVersionString(EAKBKHircType type, uint32_t version)
    {
        return version <= 125 ? ToV125String(type) : NameOf(type);
    }

    // C#'s `this byte rawType` extension. Returns EAKBKHircType(0) -- not a declared member -- for a v125
    // raw byte with no current counterpart, exactly as C#'s `_ => 0` arm does.
    inline EAKBKHircType MapToCurrent(uint8_t rawType, uint32_t version)
    {
        if (version > 125)
            return static_cast<EAKBKHircType>(rawType);

        switch (static_cast<EAKBKHircType_v125>(rawType))
        {
        case EAKBKHircType_v125::Settings: return EAKBKHircType::State;
        case EAKBKHircType_v125::SoundSfxVoice: return EAKBKHircType::SoundSfxVoice;
        case EAKBKHircType_v125::EventAction: return EAKBKHircType::EventAction;
        case EAKBKHircType_v125::Event: return EAKBKHircType::Event;
        case EAKBKHircType_v125::RandomSequenceContainer: return EAKBKHircType::RandomSequenceContainer;
        case EAKBKHircType_v125::SwitchContainer: return EAKBKHircType::SwitchContainer;
        case EAKBKHircType_v125::ActorMixer: return EAKBKHircType::ActorMixer;
        case EAKBKHircType_v125::AudioBus: return EAKBKHircType::AudioBus;
        case EAKBKHircType_v125::LayerContainer: return EAKBKHircType::LayerContainer;
        case EAKBKHircType_v125::MusicSegment: return EAKBKHircType::MusicSegment;
        case EAKBKHircType_v125::MusicTrack: return EAKBKHircType::MusicTrack;
        case EAKBKHircType_v125::MusicSwitchContainer: return EAKBKHircType::MusicSwitchContainer;
        case EAKBKHircType_v125::MusicRandomSequenceContainer: return EAKBKHircType::MusicRandomSequenceContainer;
        case EAKBKHircType_v125::Attenuation: return EAKBKHircType::Attenuation;
        case EAKBKHircType_v125::DialogueEvent: return EAKBKHircType::DialogueEvent;
        case EAKBKHircType_v125::FeedbackBus: return EAKBKHircType::FeedbackBus;
        case EAKBKHircType_v125::FeedbackNode: return EAKBKHircType::FeedbackNode;
        case EAKBKHircType_v125::FxShareSet: return EAKBKHircType::FxShareSet;
        case EAKBKHircType_v125::FxCustom: return EAKBKHircType::FxCustom;
        case EAKBKHircType_v125::AuxiliaryBus: return EAKBKHircType::AuxiliaryBus;
        case EAKBKHircType_v125::LFO: return EAKBKHircType::LFO;
        case EAKBKHircType_v125::Envelope: return EAKBKHircType::Envelope;
        case EAKBKHircType_v125::AudioDevice: return EAKBKHircType::AudioDevice;
        default: return static_cast<EAKBKHircType>(0);
        }
    }
}
