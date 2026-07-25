// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/UAkAcousticTexture.cs
// An acoustic texture asset; the cooked data stays a generic property bag, as in C#.
#pragma once

#include <optional>

#include "../../Objects/FStructFallback.h"
#include "../../Readers/FAssetArchive.h"
#include "UAkAudioType.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;

    class UAkAcousticTexture : public UAkAudioType
    {
    public:
        std::optional<FStructFallback> AcousticTextureCookedData;

        void Deserialize(FAssetArchive& Ar, int64_t validPos) override
        {
            UAkAudioType::Deserialize(Ar, validPos);

            if (Ar.Position >= validPos) return;
            AcousticTextureCookedData.emplace(Ar, std::string("WwiseAcousticTextureCookedData"));
        }
    };
}
