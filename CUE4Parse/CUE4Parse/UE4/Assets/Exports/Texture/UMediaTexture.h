// Ported from CUE4Parse/UE4/Assets/Exports/Texture/UMediaTexture.cs
// A media player's output surface. Derives from UTexture directly (not UTexture2D) and adds nothing.
#pragma once

#include "UTexture.h"

namespace CUE4Parse::UE4::Assets::Exports::Texture
{
    class UMediaTexture : public UTexture
    {
    };
}
