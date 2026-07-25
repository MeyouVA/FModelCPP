// Ported from CUE4Parse/UE4/Wwise/Objects/AkMusicTransitionRule.cs
#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "../WwiseArchive.h"
#include "../Enums/EAkCurveInterpolation.h"
#include "../Enums/EAkEntryType.h"
#include "../Enums/EAkJumpToSelType.h"
#include "../Enums/EAkSyncType.h"
#include "AkMusicFade.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    using CUE4Parse::UE4::Wwise::Enums::EAkCurveInterpolation;
    using CUE4Parse::UE4::Wwise::Enums::EAkEntryType;
    using CUE4Parse::UE4::Wwise::Enums::EAkJumpToSelType;
    using CUE4Parse::UE4::Wwise::Enums::EAkSyncType;

    struct AkMusicTransSrcRule
    {
        int32_t TransitionTime = 0;
        EAkCurveInterpolation FadeCurve = static_cast<EAkCurveInterpolation>(0);
        int32_t FadeOffset = 0;
        EAkSyncType SyncType = static_cast<EAkSyncType>(0);
        uint32_t MarkerId = 0;
        uint32_t CueFilterHash = 0;
        bool PlayPostExit = false;

        AkMusicTransSrcRule() = default;

        explicit AkMusicTransSrcRule(FWwiseArchive& Ar)
        {
            TransitionTime = Ar.Read<int32_t>();
            FadeCurve = static_cast<EAkCurveInterpolation>(Ar.Read<uint32_t>());
            FadeOffset = Ar.Read<int32_t>();
            SyncType = Ar.Read<EAkSyncType>();

            // Versions <= 62 read neither field.
            if (Ar.Version > 62 && Ar.Version <= 72)
                MarkerId = Ar.Read<uint32_t>();
            else if (Ar.Version > 72)
                CueFilterHash = Ar.Read<uint32_t>();

            PlayPostExit = Ar.ReadBool();
        }
    };

    struct AkMusicTransDestRule
    {
        int32_t TransitionTime = 0;
        EAkCurveInterpolation FadeCurve = static_cast<EAkCurveInterpolation>(0);
        int32_t FadeOffset = 0;
        uint32_t MarkerId = 0;
        uint32_t CueFilterHash = 0;
        uint32_t JumpToId = 0;
        EAkJumpToSelType JumpToType = static_cast<EAkJumpToSelType>(0);
        EAkEntryType EntryType = static_cast<EAkEntryType>(0);
        bool PlayPreEntry = false;
        bool DestMatchSourceCueName = false;

        AkMusicTransDestRule() = default;

        explicit AkMusicTransDestRule(FWwiseArchive& Ar)
        {
            TransitionTime = Ar.Read<int32_t>();
            FadeCurve = static_cast<EAkCurveInterpolation>(Ar.Read<uint32_t>());
            FadeOffset = Ar.Read<int32_t>();

            if (Ar.Version <= 72)
                MarkerId = Ar.Read<uint32_t>();
            else
                CueFilterHash = Ar.Read<uint32_t>();

            JumpToId = Ar.Read<uint32_t>();

            if (Ar.Version > 132)
                JumpToType = static_cast<EAkJumpToSelType>(Ar.Read<uint16_t>());

            EntryType = static_cast<EAkEntryType>(Ar.Read<uint16_t>());
            PlayPreEntry = Ar.ReadBool();

            if (Ar.Version > 62)
                DestMatchSourceCueName = Ar.ReadBool();
        }
    };

    struct AkMusicTransitionObject
    {
        uint32_t SegmentId = 0;
        AkMusicFade FadeInParams;
        AkMusicFade FadeOutParams;
        bool PlayPreEntry = false;
        bool PlayPostExit = false;

        AkMusicTransitionObject() = default;

        explicit AkMusicTransitionObject(FWwiseArchive& Ar)
        {
            SegmentId = Ar.Read<uint32_t>();
            FadeInParams = AkMusicFade(Ar);
            FadeOutParams = AkMusicFade(Ar);
            PlayPreEntry = Ar.ReadBool();
            PlayPostExit = Ar.ReadBool();
        }
    };

    struct TransitionRule
    {
        std::vector<int32_t> SrcIds;
        std::vector<int32_t> DestIds;
        AkMusicTransSrcRule SrcRules;
        AkMusicTransDestRule DestRules;
        std::optional<AkMusicTransitionObject> TransObject;

        TransitionRule() = default;

        explicit TransitionRule(FWwiseArchive& Ar)
        {
            const int numSrc = Ar.Version <= 72 ? 1 : Ar.Read<int32_t>();
            SrcIds = Ar.ReadArray<int32_t>(numSrc);

            const int numDest = Ar.Version <= 72 ? 1 : Ar.Read<int32_t>();
            DestIds = Ar.ReadArray<int32_t>(numDest);

            SrcRules = AkMusicTransSrcRule(Ar);
            DestRules = AkMusicTransDestRule(Ar);

            bool hasTransitionObject;
            if (Ar.Version <= 72)
            {
                Ar.ReadBool(); // bIsTransObjectEnabled
                hasTransitionObject = true; // No, don't use bool above, trust me
            }
            else
            {
                hasTransitionObject = Ar.ReadBool();
            }

            if (hasTransitionObject)
            {
                TransObject = AkMusicTransitionObject(Ar);
            }
        }
    };

    struct AkMusicTransitionRule
    {
        std::vector<TransitionRule> Rules;

        AkMusicTransitionRule() = default;

        explicit AkMusicTransitionRule(FWwiseArchive& Ar)
        {
            const int count = static_cast<int>(Ar.Read<uint32_t>());
            Rules = Ar.ReadArrayWith(count, [&Ar] { return TransitionRule(Ar); });
        }
    };
}
