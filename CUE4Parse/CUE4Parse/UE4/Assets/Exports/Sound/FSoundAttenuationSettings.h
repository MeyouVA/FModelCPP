// Ported from CUE4Parse/UE4/Assets/Exports/Sound/FSoundAttenuationSettings.cs
// [StructFallback]. Adds nothing to the base yet, exactly as in C#.
#pragma once

#include "FBaseAttenuationSettings.h"

namespace CUE4Parse::UE4::Assets::Exports::Sound
{
    class FSoundAttenuationSettings : public FBaseAttenuationSettings
    {
    public:
        FSoundAttenuationSettings() = default;
        explicit FSoundAttenuationSettings(const FStructFallback& fallback) : FBaseAttenuationSettings(fallback) {}
    };
}
