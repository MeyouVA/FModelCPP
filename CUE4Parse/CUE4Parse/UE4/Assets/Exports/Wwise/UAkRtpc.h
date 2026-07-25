// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/UAkRtpc.cs
// A game parameter (RTPC) asset; the cooked data stays a generic property bag, as in C#.
#pragma once

#include <optional>

#include "../../Objects/FStructFallback.h"
#include "../../Readers/FAssetArchive.h"
#include "UAkAudioType.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;

    class UAkRtpc : public UAkAudioType
    {
    public:
        std::optional<FStructFallback> GameParameterCookedData;

        void Deserialize(FAssetArchive& Ar, int64_t validPos) override
        {
            UAkAudioType::Deserialize(Ar, validPos);

            if (Ar.Position >= validPos) return;
            GameParameterCookedData.emplace(Ar, std::string("WwiseGameParameterCookedData"));
        }
    };
}
