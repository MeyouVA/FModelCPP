// Ported from CUE4Parse/UE4/FMod/Metadata/FHashInfo.cs
#pragma once

#include "../Objects/FModGuid.h"

namespace CUE4Parse::UE4::FMod::Metadata
{
    struct FHashInfo
    {
        Objects::FModGuid Guid;
        uint32_t Hash = 0;

        FHashInfo() = default;
        explicit FHashInfo(Readers::FArchive& Ar) : Guid(Ar)
        {
            Hash = Ar.Read<uint32_t>();
        }
    };
}
