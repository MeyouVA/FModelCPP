// Ported from CUE4Parse/UE4/FMod/Objects/FUserPropertyFloat.cs
#pragma once

#include <string>

#include "../FModReader.h"

namespace CUE4Parse::UE4::FMod::Objects
{
    struct FUserPropertyFloat
    {
        std::string Name;
        float Value = 0.0f;

        FUserPropertyFloat() = default;
        explicit FUserPropertyFloat(Readers::FArchive& Ar)
        {
            Name = FModReader::ReadString(Ar);
            Value = Ar.Read<float>();
        }
    };
}
