// Ported from CUE4Parse/UE4/Assets/Exports/Material/ETranslucencyLightingMode.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Assets::Exports::Material
{
    enum class ETranslucencyLightingMode : uint8_t
    {
        TLM_VolumetricNonDirectional,
        TLM_VolumetricDirectional,
        TLM_VolumetricPerVertexNonDirectional,
        TLM_VolumetricPerVertexDirectional,
        TLM_Surface,
        TLM_SurfacePerPixelLighting,
        TLM_MAX,
    };

    // C#'s ETranslucencyLightingMode.GetDescription() -- the [Description] attribute on each member.
    // FModel uses these as display text and as internationalisation lookup keys, so they are
    // carried over rather than dropped. Returns nullptr for a member with no [Description]
    // (C#'s extension falls back to the member name in that case).
    inline const char* Description(ETranslucencyLightingMode value)
    {
        switch (value)
        {
        case ETranslucencyLightingMode::TLM_VolumetricNonDirectional: return "Volumetric NonDirectional";
        case ETranslucencyLightingMode::TLM_VolumetricDirectional: return "Volumetric Directional";
        case ETranslucencyLightingMode::TLM_VolumetricPerVertexNonDirectional: return "Volumetric PerVertex NonDirectional";
        case ETranslucencyLightingMode::TLM_VolumetricPerVertexDirectional: return "Volumetric PerVertex Directional";
        case ETranslucencyLightingMode::TLM_Surface: return "Surface TranslucencyVolume";
        case ETranslucencyLightingMode::TLM_SurfacePerPixelLighting: return "Surface ForwardShading";
        }
        return nullptr;
    }
}
