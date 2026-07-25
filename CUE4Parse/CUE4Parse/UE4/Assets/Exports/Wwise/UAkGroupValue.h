// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/UAkGroupValue.cs
// A switch/state value asset. The cooked data has no dedicated reader in C# either -- it stays a generic property bag.
#pragma once

#include <optional>

#include "../../Objects/FStructFallback.h"
#include "../../Readers/FAssetArchive.h"
#include "UAkAudioType.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;

    class UAkGroupValue : public UAkAudioType
    {
    public:
        std::optional<FStructFallback> GroupValueCookedData;

        void Deserialize(FAssetArchive& Ar, int64_t validPos) override
        {
            UAkAudioType::Deserialize(Ar, validPos);

            if (Ar.Position >= validPos) return;
            GroupValueCookedData.emplace(Ar, std::string("WwiseGroupValueCookedData"));
        }
    };
}
