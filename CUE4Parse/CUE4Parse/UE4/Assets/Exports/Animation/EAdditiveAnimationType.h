// Ported from CUE4Parse/UE4/Assets/Exports/Animation/EAdditiveAnimationType.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Assets::Exports::Animation
{
    enum class EAdditiveAnimationType
    {
        AAT_None,
        AAT_LocalSpaceBase,
        AAT_RotationOffsetMeshSpace,
        AAT_MAX,
    };
}
