// Ported from CUE4Parse/UE4/Assets/Exports/Material/EMaterialShadingModel.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Assets::Exports::Material
{
    enum class EMaterialShadingModel
    {
        MSM_Unlit,
        MSM_DefaultLit,
        MSM_Subsurface,
        MSM_PreintegratedSkin,
        MSM_ClearCoat,
        MSM_SubsurfaceProfile,
        MSM_TwoSidedFoliage,
        MSM_Hair,
        MSM_Cloth,
        MSM_Eye,
        MSM_SingleLayerWater,
        MSM_ThinTranslucent,
        MSM_Strata,
        // Number of unique shading models.
        MSM_NUM,
        // Shading model will be determined by the Material Expression Graph,
        // by utilizing the 'Shading Model' MaterialAttribute output pin.
        MSM_FromMaterialExpression,
        MSM_MAX,
    };

    // C#'s EMaterialShadingModel.GetDescription() -- the [Description] attribute on each member.
    // FModel uses these as display text and as internationalisation lookup keys, so they are
    // carried over rather than dropped. Returns nullptr for a member with no [Description]
    // (C#'s extension falls back to the member name in that case).
    inline const char* Description(EMaterialShadingModel value)
    {
        switch (value)
        {
        case EMaterialShadingModel::MSM_Unlit: return "Unlit";
        case EMaterialShadingModel::MSM_DefaultLit: return "Default Lit";
        case EMaterialShadingModel::MSM_Subsurface: return "Subsurface";
        case EMaterialShadingModel::MSM_PreintegratedSkin: return "Preintegrated Skin";
        case EMaterialShadingModel::MSM_ClearCoat: return "Clear Coat";
        case EMaterialShadingModel::MSM_SubsurfaceProfile: return "Subsurface Profile";
        case EMaterialShadingModel::MSM_TwoSidedFoliage: return "Two Sided Foliage";
        case EMaterialShadingModel::MSM_Hair: return "Hair";
        case EMaterialShadingModel::MSM_Cloth: return "Cloth";
        case EMaterialShadingModel::MSM_Eye: return "Eye";
        case EMaterialShadingModel::MSM_SingleLayerWater: return "SingleLayerWater";
        case EMaterialShadingModel::MSM_ThinTranslucent: return "Thin Translucent";
        case EMaterialShadingModel::MSM_Strata: return "Strata";
        case EMaterialShadingModel::MSM_NUM: return "NUM";
        case EMaterialShadingModel::MSM_FromMaterialExpression: return "From Material Expression";
        case EMaterialShadingModel::MSM_MAX: return "MAX";
        }
        return nullptr;
    }
}
