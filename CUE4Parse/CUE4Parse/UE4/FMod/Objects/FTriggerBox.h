// Ported from CUE4Parse/UE4/FMod/Objects/FTriggerBox.cs
#pragma once

#include "FModGuid.h"

namespace CUE4Parse::UE4::FMod::Objects
{
    struct FTriggerBox
    {
        FModGuid Guid;
        uint32_t StartTime = 0;
        uint32_t Length = 0;

        FTriggerBox() = default;
        explicit FTriggerBox(Readers::FArchive& Ar) : Guid(Ar)
        {
            StartTime = Ar.Read<uint32_t>();
            Length = Ar.Read<uint32_t>();
        }
    };
}
