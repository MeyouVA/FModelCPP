// Ported from CUE4Parse/UE4/FMod/Objects/FQuantization.cs
#pragma once

#include "../../Readers/FArchive.h"
#include "../Enums/EQuantizationUnit.h"

namespace CUE4Parse::UE4::FMod::Objects
{
    struct FQuantization
    {
        Enums::EQuantizationUnit Unit{};
        int32_t Multiplier = 0;

        FQuantization() = default;
        explicit FQuantization(Readers::FArchive& Ar)
        {
            Unit = static_cast<Enums::EQuantizationUnit>(Ar.Read<uint32_t>());

            if (Unit <= Enums::EQuantizationUnit::EighthNote)
                Multiplier = Ar.Read<int32_t>();
            else
                Multiplier = 0;
        }
    };
}
