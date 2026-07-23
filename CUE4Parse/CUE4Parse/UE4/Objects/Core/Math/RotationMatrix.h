// Ported from CUE4Parse/UE4/Objects/Core/Math/RotationMatrix.cs — a pure rotation matrix (no translation),
// i.e. FRotationTranslationMatrix with a zero origin.
#pragma once

#include "RotationTranslationMatrix.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    struct FRotator; // FRotator.h

    class FRotationMatrix : public FRotationTranslationMatrix
    {
    public:
        explicit FRotationMatrix(const FRotator& rot) : FRotationTranslationMatrix(rot, FVector()) {}
    };
}
