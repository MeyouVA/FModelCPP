// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/UAkAudioDeviceShareSet.cs
// An audio-device share set; the cooked data stays a generic property bag, as in C#.
#pragma once

#include <optional>

#include "../../Objects/FStructFallback.h"
#include "../../Readers/FAssetArchive.h"
#include "UAkAudioType.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;

    class UAkAudioDeviceShareSet : public UAkAudioType
    {
    public:
        std::optional<FStructFallback> AudioDeviceShareSetCookedData;

        void Deserialize(FAssetArchive& Ar, int64_t validPos) override
        {
            UAkAudioType::Deserialize(Ar, validPos);

            if (Ar.Position >= validPos) return;
            AudioDeviceShareSetCookedData.emplace(Ar, std::string("WwiseAudioDeviceShareSetCookedData"));
        }
    };
}
