// Ported from CUE4Parse/UE4/FMod/Objects/FPlaylistEntry.cs
#pragma once

#include "FModGuid.h"

namespace CUE4Parse::UE4::FMod::Objects
{
    struct FPlaylistEntry
    {
        FModGuid Guid;
        float Weight = 0.0f;

        FPlaylistEntry() = default;
        explicit FPlaylistEntry(Readers::FArchive& Ar) : Guid(Ar)
        {
            Weight = Ar.Read<float>();
        }
    };
}
