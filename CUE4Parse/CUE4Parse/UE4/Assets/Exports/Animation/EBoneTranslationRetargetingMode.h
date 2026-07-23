// Ported from CUE4Parse/UE4/Assets/Exports/Animation/EBoneTranslationRetargetingMode.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Assets::Exports::Animation
{
    enum class EBoneTranslationRetargetingMode : uint8_t
    {
        // Use translation from animation data.
        Animation,
        // Use fixed translation from Skeleton.
        Skeleton,
        // Use Translation from animation, but scale length by Skeleton's proportions.
        AnimationScaled,
        // Use Translation from animation, but also play the difference from retargeting pose as an additive.
        AnimationRelative,
        // Apply delta orientation and scale from ref pose
        OrientAndScale,
    };
}
