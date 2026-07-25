// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/UAkAudioBank.cs
// A soundbank asset. The `validPos - 4` guard (not `validPos`, as its siblings use) is C#'s, and kept.
//
// The five game-specific early-outs and the two FRawHeader branches C# has need the unported GameTypes and
// raw-header layers, so this port takes the default arm for every game. TODO with those layers.
// The two game-specific subclasses declared in the same C# file are kept.
#pragma once

#include <optional>

#include "../../Objects/FStructFallback.h"
#include "../../Readers/FAssetArchive.h"
#include "UAkAudioType.h"
#include "FWwiseLocalizedSoundBankCookedData.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;

    class UAkAudioBank : public UAkAudioType
    {
    public:
        std::optional<FWwiseLocalizedSoundBankCookedData> SoundBankCookedData;

        void Deserialize(FAssetArchive& Ar, int64_t validPos) override
        {
            UAkAudioType::Deserialize(Ar, validPos);

            if (Ar.Position >= validPos - 4) return;
            SoundBankCookedData.emplace(FStructFallback(Ar, std::string("WwiseLocalizedSoundBankCookedData")));
        }
    };

    class UWuiBank : public UAkAudioBank {};   // The Awesome Adventures of Captain Spirit
    class UWwiseBank : public UAkAudioBank {}; // Borderlands 3
}
