// Ported from CUE4Parse/UE4/Objects/Core/Math/QuatRotationTranslationMatrix.cs — rotation (from a quaternion)
// plus translation matrix, and its zero-translation sibling. The ctor lives in FQuat.cpp (needs FQuat complete).
#pragma once

#include "Matrix.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    struct FQuat; // FQuat.h

    class FQuatRotationTranslationMatrix : public FMatrix
    {
    public:
        FQuatRotationTranslationMatrix(const FQuat& q, const FVector& origin);
    };

    class FQuatRotationMatrix : public FQuatRotationTranslationMatrix
    {
    public:
        explicit FQuatRotationMatrix(const FQuat& q) : FQuatRotationTranslationMatrix(q, FVector::ZeroVector) {}
    };
}
