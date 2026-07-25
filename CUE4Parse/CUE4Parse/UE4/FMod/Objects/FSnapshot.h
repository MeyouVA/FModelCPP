// Ported from CUE4Parse/UE4/FMod/Objects/FSnapshot.cs
#pragma once

#include "FModGuid.h"

namespace CUE4Parse::UE4::FMod::Objects
{
    struct FSnapshot
    {
        FModGuid SnapshotGuid;
        uint32_t EntryIndex = 0;
        uint32_t TargetIndex = 0;
        float Value = 0.0f;

        FSnapshot() = default;
        explicit FSnapshot(Readers::FArchive& Ar) : SnapshotGuid(Ar)
        {
            EntryIndex = Ar.Read<uint32_t>();
            TargetIndex = Ar.Read<uint32_t>();
            Value = Ar.Read<float>();
        }
    };
}
