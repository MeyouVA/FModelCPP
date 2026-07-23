// Ported from CUE4Parse/UE4/Assets/Exports/Material/EBlendMode.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Assets::Exports::Material
{
    enum class EBlendMode : uint8_t
    {
        BLEND_Opaque,
        BLEND_Masked,
        BLEND_Translucent,
        BLEND_Additive,
        BLEND_Modulate,
        BLEND_AlphaComposite,
        BLEND_AlphaHoldout,
        BLEND_TranslucentColoredTransmittance,
        BLEND_MAX,
        BLEND_TranslucentGreyTransmittance    = BLEND_Translucent,
        BLEND_ColoredTransmittanceOnly        = BLEND_Modulate,
    };

    // C#'s EBlendMode.GetDescription() -- the [Description] attribute on each member.
    // FModel uses these as display text and as internationalisation lookup keys, so they are
    // carried over rather than dropped. Returns nullptr for a member with no [Description]
    // (C#'s extension falls back to the member name in that case).
    inline const char* Description(EBlendMode value)
    {
        switch (value)
        {
        case EBlendMode::BLEND_Opaque: return "Opaque";
        case EBlendMode::BLEND_Masked: return "Masked";
        case EBlendMode::BLEND_Translucent: return "Translucent";
        case EBlendMode::BLEND_Additive: return "Additive";
        case EBlendMode::BLEND_Modulate: return "Modulate";
        case EBlendMode::BLEND_AlphaComposite: return "AlphaComposite (Premultiplied Alpha)";
        case EBlendMode::BLEND_AlphaHoldout: return "AlphaHoldout";
        case EBlendMode::BLEND_MAX: return "MAX";
        }
        return nullptr;
    }
}
