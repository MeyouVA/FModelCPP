// Ported from CUE4Parse/UE4/Assets/Exports/Texture/TextureAddress.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Assets::Exports::Texture
{
    enum class TextureAddress : uint8_t
    {
        TA_Wrap,
        TA_Clamp,
        TA_Mirror,
        TA_MAX,
    };

    // C#'s TextureAddress.GetDescription() -- the [Description] attribute on each member.
    // FModel uses these as display text and as internationalisation lookup keys, so they are
    // carried over rather than dropped. Returns nullptr for a member with no [Description]
    // (C#'s extension falls back to the member name in that case).
    inline const char* Description(TextureAddress value)
    {
        switch (value)
        {
        case TextureAddress::TA_Wrap: return "Wrap";
        case TextureAddress::TA_Clamp: return "Clamp";
        case TextureAddress::TA_Mirror: return "Mirror";
        }
        return nullptr;
    }
}
