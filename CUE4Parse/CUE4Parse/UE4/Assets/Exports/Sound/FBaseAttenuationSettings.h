// Ported from CUE4Parse/UE4/Assets/Exports/Sound/FBaseAttenuationSettings.cs
// [StructFallback]. C# reads only FalloffDistance out of a much larger UE struct; so does the port.
#pragma once

#include "../PropertyUtil.h"

namespace CUE4Parse::UE4::Assets::Exports::Sound
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;

    class FBaseAttenuationSettings
    {
    public:
        float FalloffDistance = 0.0f;

        FBaseAttenuationSettings() = default;
        virtual ~FBaseAttenuationSettings() = default;

        explicit FBaseAttenuationSettings(const FStructFallback& fallback)
        {
            FalloffDistance = PropertyUtil::GetOrDefault<float>(fallback, "FalloffDistance");
        }
    };
}
