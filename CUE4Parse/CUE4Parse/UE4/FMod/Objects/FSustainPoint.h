// Ported from CUE4Parse/UE4/FMod/Objects/FSustainPoint.cs
#pragma once

#include <vector>

#include "FEvaluator.h"

namespace CUE4Parse::UE4::FMod::Objects
{
    struct FSustainPoint
    {
        uint32_t Position = 0;
        std::vector<FEvaluator> Evaluators;

        FSustainPoint() = default;
        explicit FSustainPoint(Readers::FArchive& Ar)
        {
            Position = Ar.Read<uint32_t>();
            Evaluators = FEvaluator::ReadEvaluatorList(Ar);
        }
    };
}
