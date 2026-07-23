// Ported from CUE4Parse/UE4/Assets/Exports/Texture/ETexturePlatform.cs
#pragma once

namespace CUE4Parse::UE4::Assets::Exports::Texture
{
    enum class ETexturePlatform
    {
        DesktopMobile,       // Desktop / Mobile
        XboxAndPlaystation4, // Xbox One/Series / Playstation 4
        NintendoSwitch,      // Nintendo Switch
        Playstation5         // Playstation 5
    };

    // C#'s ETexturePlatform.GetDescription() -- the [Description] attribute on each member.
    // FModel uses these as display text and as internationalisation lookup keys, so they are carried over
    // rather than left as comments. Returns nullptr for a member with no [Description] (C#'s extension
    // falls back to the member name in that case).
    inline const char* Description(ETexturePlatform value)
    {
        switch (value)
        {
        case ETexturePlatform::DesktopMobile: return "Desktop / Mobile";
        case ETexturePlatform::XboxAndPlaystation4: return "Xbox One/Series / Playstation 4";
        case ETexturePlatform::NintendoSwitch: return "Nintendo Switch";
        case ETexturePlatform::Playstation5: return "Playstation 5";
        }
        return nullptr;
    }
}
