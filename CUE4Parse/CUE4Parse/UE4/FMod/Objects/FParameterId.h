// Ported from CUE4Parse/UE4/FMod/Objects/FParameterId.cs
#pragma once

#include "../../Readers/FArchive.h"

namespace CUE4Parse::UE4::FMod::Objects
{
    struct FParameterId
    {
        uint32_t Data1 = 0;
        uint32_t Data2 = 0;

        FParameterId() = default;
        explicit FParameterId(Readers::FArchive& Ar)
        {
            Data1 = Ar.Read<uint32_t>();
            Data2 = Ar.Read<uint32_t>();
        }
    };
}
