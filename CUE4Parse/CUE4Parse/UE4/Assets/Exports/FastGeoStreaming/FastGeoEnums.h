// Ported from CUE4Parse/UE4/Assets/Exports/FastGeoStreaming/FastGeoEnums.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Assets::Exports::FastGeoStreaming
{
    enum class ESkinCacheUsage : uint8_t
    {
        Auto     = 0,
        Disabled = 255,
        Enabled  = 1,
    };

    enum class EDetailMode : uint8_t
    {
        Low    = 0,
        Medium = 1,
        High   = 2,
        Epic   = 3,
    };

    enum class EHasCustomNavigableGeometry : uint8_t
    {
        No,
        Yes,
        EvenIfNotCollidable,
        DontExport,
    };

    enum class EComponentMobility : uint8_t
    {
        Static,
        Stationary,
        Movable,
    };

    enum class ELightmapType : uint8_t
    {
        Default,
        ForceSurface,
        ForceVolumetric,
    };

    enum class ESceneDepthPriorityGroup : uint8_t
    {
        World,
        Foreground,
    };

    enum class ERendererStencilMask : uint8_t
    {
        ERSM_Default,
        ERSM_255,
        ERSM_1,
        ERSM_2,
        ERSM_4,
        ERSM_8,
        ERSM_16,
        ERSM_32,
        ERSM_64,
        ERSM_128,
    };

    enum class ERayTracingGroupCullingPriority : uint8_t
    {
        CP_0_NEVER_CULL,
        CP_1,
        CP_2,
        CP_3,
        CP_4_DEFAULT,
        CP_5,
        CP_6,
        CP_7,
        CP_8_QUICKLY_CULL,
    };

    enum class EIndirectLightingCacheQuality : uint8_t
    {
        Off,
        Point,
        Volume,
    };

    enum class EShadowCacheInvalidationBehavior : uint8_t
    {
        Auto,
        Always,
        Rigid,
        Static,
    };

    enum class ERuntimeVirtualTextureMainPassType : uint8_t
    {
        Never,
        Exclusive,
        Always,
    };
}
