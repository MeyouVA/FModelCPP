// Ported from CUE4Parse/UE4/Assets/Exports/Component/TextRender/EVerticalTextAligment.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Assets::Exports::Component::TextRender
{
    enum class EVerticalTextAligment : int32_t
    {
        EVRTA_TextTop,
        EVRTA_TextCenter,
        EVRTA_TextBottom,
        EVRTA_QuadTop,
    };

    // C#'s EVerticalTextAligment.GetDescription() -- the [Description] attribute on each member.
    // FModel uses these as display text and as internationalisation lookup keys, so they are
    // carried over rather than dropped. Returns nullptr for a member with no [Description]
    // (C#'s extension falls back to the member name in that case).
    inline const char* Description(EVerticalTextAligment value)
    {
        switch (value)
        {
        case EVerticalTextAligment::EVRTA_TextTop: return "Text Top";
        case EVerticalTextAligment::EVRTA_TextCenter: return "Text Center";
        case EVerticalTextAligment::EVRTA_TextBottom: return "Text Bottom";
        case EVerticalTextAligment::EVRTA_QuadTop: return "Quad Top";
        }
        return nullptr;
    }
}
