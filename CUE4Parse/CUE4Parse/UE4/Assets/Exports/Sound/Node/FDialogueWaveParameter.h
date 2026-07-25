// Ported from CUE4Parse/UE4/Assets/Exports/Sound/Node/FDialogueWaveParameter.cs
// [StructFallback]. The dialogue wave a player node points at, plus the context to pick from it.
#pragma once

#include "FDialogueContext.h"

namespace CUE4Parse::UE4::Assets::Exports::Sound::Node
{
    struct FDialogueWaveParameter
    {
        FPackageIndex DialogueWave;
        FDialogueContext Context;

        FDialogueWaveParameter() = default;

        explicit FDialogueWaveParameter(const FStructFallback& fallback)
        {
            DialogueWave = PropertyUtil::GetOrDefault<FPackageIndex>(fallback, "DialogueWave");
            Context = PropertyUtil::GetOrDefault<FDialogueContext>(fallback, "Context");
        }
    };
}
