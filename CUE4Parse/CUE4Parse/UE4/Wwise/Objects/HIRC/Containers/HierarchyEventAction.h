// Ported from CUE4Parse/UE4/Wwise/Objects/HIRC/Containers/HierarchyEventAction.cs
#pragma once

#include <cstdint>
#include <memory>

#include "../../../WwiseArchive.h"
#include "../../../Enums/EAkActionScope.h"
#include "../../../Enums/EAkActionType.h"
#include "../../AkPropBundle.h"
#include "../../Actions/CAkActionBase.h"
#include "../../Actions/CAkActionBypassFX.h"
#include "../../Actions/CAkActionPause.h"
#include "../../Actions/CAkActionPlay.h"
#include "../../Actions/CAkActionResume.h"
#include "../../Actions/CAkActionSeek.h"
#include "../../Actions/CAkActionSetAkProp.h"
#include "../../Actions/CAkActionSetFX.h"
#include "../../Actions/CAkActionSetGameParameter.h"
#include "../../Actions/CAkActionSetState.h"
#include "../../Actions/CAkActionSetSwitch.h"
#include "../../Actions/CAkActionStop.h"
#include "../AbstractHierarchy.h"

namespace CUE4Parse::UE4::Wwise::Objects::HIRC::Containers
{
    using CUE4Parse::UE4::Wwise::Enums::EAkActionScope;
    using CUE4Parse::UE4::Wwise::Enums::EAkActionType;
    namespace Actions = CUE4Parse::UE4::Wwise::Objects::Actions;

    // CAkAction::Create
    // CAkAction::SetInitialValues
    class HierarchyEventAction : public AbstractHierarchy
    {
    public:
        // C# types ActionData as `object?` -- the action classes share no base. The port gives them one
        // so the payload can be owned polymorphically; ActionKind says which type is actually there.
        enum class EActionKind
        {
            None,
            Play, Stop, SetGameParameter, SetAkProp, Seek, SetSwitch, SetState, SetFX,
            Base, Resume, Pause, BypassFX
        };

        class IActionData
        {
        public:
            virtual ~IActionData() = default;
        };

        template <typename T>
        class ActionData final : public IActionData
        {
        public:
            T Value;
            explicit ActionData(FWwiseArchive& Ar) : Value(Ar) {}
        };

        EAkActionScope EventActionScope = static_cast<EAkActionScope>(0);
        EAkActionType EventActionType = static_cast<EAkActionType>(0);
        bool IsBus = false;
        uint32_t ReferencedId = 0;
        AkPropBundle PropBundle;
        EActionKind ActionKind = EActionKind::None;
        std::unique_ptr<IActionData> ActionData_;

        explicit HierarchyEventAction(FWwiseArchive& Ar)
        {
            Id = Ar.Read<uint32_t>();
            EventActionScope = Ar.Read<EAkActionScope>();
            EventActionType = Ar.Read<EAkActionType>();
            ReferencedId = Ar.Read<uint32_t>();
            if (Ar.Version > 65)
            {
                IsBus = Ar.ReadBool();
            }

            PropBundle = AkPropBundle(Ar);

            switch (EventActionType)
            {
                case EAkActionType::Play:
                    Make<Actions::CAkActionPlay>(Ar, EActionKind::Play); break;
                case EAkActionType::Stop:
                    Make<Actions::CAkActionStop>(Ar, EActionKind::Stop); break;

                case EAkActionType::SetGameParameter:
                case EAkActionType::ResetGameParameter:
                    Make<Actions::CAkActionSetGameParameter>(Ar, EActionKind::SetGameParameter); break;

                case EAkActionType::SetHighPassFilter:
                case EAkActionType::ResetHighPassFilter:
                case EAkActionType::ResetVoiceLowPassFilter:
                case EAkActionType::ResetBusVolume:
                case EAkActionType::SetVoiceVolume:
                case EAkActionType::SetVoicePitch:
                case EAkActionType::SetBusVolume:
                case EAkActionType::SetVoiceLowPassFilter:
                case EAkActionType::ResetVoiceVolume:
                case EAkActionType::ResetVoicePitch:
                    Make<Actions::CAkActionSetAkProp>(Ar, EActionKind::SetAkProp); break;

                case EAkActionType::Seek:
                    Make<Actions::CAkActionSeek>(Ar, EActionKind::Seek); break;
                case EAkActionType::SetSwitch:
                    Make<Actions::CAkActionSetSwitch>(Ar, EActionKind::SetSwitch); break;
                case EAkActionType::SetState:
                    Make<Actions::CAkActionSetState>(Ar, EActionKind::SetState); break;

                case EAkActionType::SetEffect:
                case EAkActionType::ResetEffect:
                    Make<Actions::CAkActionSetFX>(Ar, EActionKind::SetFX); break;

                case EAkActionType::Mute:
                case EAkActionType::UnMute:
                case EAkActionType::ResetPlaylist:
                    Make<Actions::CAkActionBase>(Ar, EActionKind::Base); break;

                case EAkActionType::Resume:
                    Make<Actions::CAkActionResume>(Ar, EActionKind::Resume); break;
                case EAkActionType::Pause:
                    Make<Actions::CAkActionPause>(Ar, EActionKind::Pause); break;

                // Break and Trigger only carry a bypass-FX payload below bank version 150 -- the C#
                // switch matches on (type, version) for these two alone.
                case EAkActionType::Break:
                case EAkActionType::Trigger:
                    if (Ar.Version < 150) Make<Actions::CAkActionBypassFX>(Ar, EActionKind::BypassFX);
                    break;

                case EAkActionType::SetBypassEffectSlot:
                case EAkActionType::SetBypassAllEffects:
                case EAkActionType::ResetBypassEffectSlot:
                case EAkActionType::ResetBypassEffects:
                    Make<Actions::CAkActionBypassFX>(Ar, EActionKind::BypassFX); break;

                // TODO: add all action types
                default: break;
            }
        }

        // Typed accessor: returns nullptr if the payload is not a T.
        template <typename T>
        const T* Get() const
        {
            const auto* typed = dynamic_cast<const ActionData<T>*>(ActionData_.get());
            return typed ? &typed->Value : nullptr;
        }

    private:
        template <typename T>
        void Make(FWwiseArchive& Ar, EActionKind kind)
        {
            ActionData_ = std::make_unique<ActionData<T>>(Ar);
            ActionKind = kind;
        }
    };
}
