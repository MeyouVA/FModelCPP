// Ported from CUE4Parse/UE4/Objects/Core/Math/FTwoVectors.cs.
#pragma once

#include <string>

#include "FVector.h"
#include "../../../Readers/FArchive.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    struct FTwoVectors
    {
        FVector V1;
        FVector V2;

        FTwoVectors() = default;
        FTwoVectors(FVector v1, FVector v2) : V1(v1), V2(v2) {}
        explicit FTwoVectors(CUE4Parse::UE4::Readers::FArchive& Ar) : V1(Ar), V2(Ar) {}

        std::string ToString() const { return "V1: " + V1.ToString() + ", V2: " + V2.ToString(); }
    };
}
