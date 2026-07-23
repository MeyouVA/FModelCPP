// Ported from CUE4Parse/UE4/Assets/Exports/Component/TextRender/EHorizTextAligment.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Assets::Exports::Component::TextRender
{
    enum class EHorizTextAligment : int32_t
    {
        EHTA_Left,
        EHTA_Center,
        EHTA_Right,
    };

    // C#'s EHorizTextAligment.GetDescription() -- the [Description] attribute on each member.
    // FModel uses these as display text and as internationalisation lookup keys, so they are
    // carried over rather than dropped. Returns nullptr for a member with no [Description]
    // (C#'s extension falls back to the member name in that case).
    inline const char* Description(EHorizTextAligment value)
    {
        switch (value)
        {
        case EHorizTextAligment::EHTA_Left: return "Left";
        case EHorizTextAligment::EHTA_Center: return "Center";
        case EHorizTextAligment::EHTA_Right: return "Right";
        }
        return nullptr;
    }
}
