// Ported from CUE4Parse/UE4/FMod/Objects/FUserPropertyString.cs
#pragma once

#include <string>

#include "../FModReader.h"

namespace CUE4Parse::UE4::FMod::Objects
{
    struct FUserPropertyString
    {
        std::string Key;
        std::string Value;

        FUserPropertyString() = default;
        explicit FUserPropertyString(Readers::FArchive& Ar)
        {
            Key = FModReader::ReadString(Ar);
            Value = FModReader::ReadString(Ar);
        }
    };
}
