// Ported from CUE4Parse/UE4/FMod/Enums/EPlaylistPlayMode.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::FMod::Enums
{
    enum class EPlaylistPlayMode : int32_t
    {
        PlaylistPlayMode_PlaySequential     = 0x0,
        PlaylistPlayMode_Random             = 0x1,
        PlaylistPlayMode_SmartRandom        = 0x2,
        PlaylistPlayMode_GlobalSequential   = 0x3,
        PlaylistPlayMode_InstanceSequential = 0x4,
        PlaylistPlayMode_Undefined          = 0x5,
        PlaylistPlayMode_Max                = 0x6,
    };
}
