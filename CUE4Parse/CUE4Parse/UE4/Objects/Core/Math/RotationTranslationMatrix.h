// Ported from CUE4Parse/UE4/Objects/Core/Math/RotationTranslationMatrix.cs — a combined rotation and
// translation matrix built from an FRotator. The ctor lives in Matrix.cpp (needs FRotator complete).
#pragma once

#include "Matrix.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    struct FRotator; // FRotator.h

    class FRotationTranslationMatrix : public FMatrix
    {
    public:
        FRotationTranslationMatrix(const FRotator& rot, const FVector& origin);
    };
}
