// Ported from CUE4Parse/UE4/Assets/Exports/Animation/AnimationKeyFormat.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Assets::Exports::Animation
{
    // Indicates animation data key format.
    enum class AnimationKeyFormat : uint8_t
    {
        AKF_ConstantKeyLerp,
        AKF_VariableKeyLerp,
        AKF_PerTrackCompression,
        AKF_MAX,
    };
}
