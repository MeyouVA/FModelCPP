// Ported from CUE4Parse/UE4/Assets/Exports/Sound/Node/FDialogueContextMapping.cs
// [StructFallback]. One context paired with the wave that voices it.
#pragma once

#include "FDialogueContext.h"

namespace CUE4Parse::UE4::Assets::Exports::Sound::Node
{
    struct FDialogueContextMapping
    {
        FDialogueContext Context;
        FPackageIndex SoundWave;

        FDialogueContextMapping() = default;

        explicit FDialogueContextMapping(const FStructFallback& fallback)
        {
            Context = PropertyUtil::GetOrDefault<FDialogueContext>(fallback, "Context");
            SoundWave = PropertyUtil::GetOrDefault<FPackageIndex>(fallback, "SoundWave");
        }
    };
}
