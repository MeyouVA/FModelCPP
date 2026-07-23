// Ported from CUE4Parse/UE4/FMod/Enums/EPlaylistSelectionMode.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::FMod::Enums
{
    enum class EPlaylistSelectionMode : int32_t
    {
        PlaylistSelectionMode_SelectOnce   = 0x0,
        PlaylistSelectionMode_SelectNormal = 0x1,
        PlaylistSelectionMode_Undefined    = 0x2,
        PlaylistSelectionMode_Max          = 0x3,
    };
}
