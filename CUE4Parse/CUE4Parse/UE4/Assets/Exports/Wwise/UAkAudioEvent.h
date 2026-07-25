// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/UAkAudioEvent.cs
// An event asset: the per-language cooked data, then four duration/attenuation fields on the wire.
//
// The two game-specific subclasses C# declares in this file (UWuiEvent for Captain Spirit, UWwiseEvent for
// Borderlands 3) are kept, as they are the names those games' packages serialize under.
#pragma once

#include <optional>

#include "../../Objects/FStructFallback.h"
#include "../../Readers/FAssetArchive.h"
#include "UAkAudioType.h"
#include "FWwiseLocalizedEventCookedData.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;

    class UAkAudioEvent : public UAkAudioType
    {
    public:
        std::optional<FWwiseLocalizedEventCookedData> EventCookedData;
        float MaximumDuration = 0.0f;
        float MinimumDuration = 0.0f;
        bool IsInfinite = false;
        float MaxAttenuationRadius = 0.0f;

        void Deserialize(FAssetArchive& Ar, int64_t validPos) override
        {
            UAkAudioType::Deserialize(Ar, validPos);

            if (Ar.Position >= validPos) return;
            EventCookedData.emplace(FStructFallback(Ar, std::string("WwiseLocalizedEventCookedData")));
            EventCookedData->SerializeBulkData(Ar);

            MaximumDuration = Ar.Read<float>();
            MinimumDuration = Ar.Read<float>();
            IsInfinite = Ar.ReadBoolean();  // four bytes, not one
            MaxAttenuationRadius = Ar.Read<float>();

            // C# additionally reads a MortalKombat1 CustomGameData map here; that member lives on the
            // unported GameTypes layer, so the branch is omitted with it. TODO.
        }
    };

    class UWuiEvent : public UAkAudioEvent {};   // The Awesome Adventures of Captain Spirit
    class UWwiseEvent : public UAkAudioEvent {}; // Borderlands 3
}
