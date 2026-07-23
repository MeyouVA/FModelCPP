// Ported from CUE4Parse/UE4/Objects/Core/Math/FHalfVector.cs — the half-precision vertex vectors.
// C#'s System.Half is FFloat16 here (see FFloat16.h for why), and its implicit conversion operators become
// explicit ToFVector() calls: C++ implicit conversions between struct types are far easier to trip over than
// C#'s, and the call sites are few.
#pragma once

#include "FFloat16.h"
#include "FVector.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    struct FHalfVectorScaled;

    struct FHalfVector
    {
        FFloat16 X;
        FFloat16 Y;
        FFloat16 Z;

        FHalfVector() = default;
        FHalfVector(FFloat16 x, FFloat16 y, FFloat16 z) : X(x), Y(y), Z(z) {}

        FVector ToFVector() const { return {X.ToFloat(), Y.ToFloat(), Z.ToFloat()}; }
        FHalfVectorScaled ToScaled() const; // defined below, once FHalfVectorScaled is complete
    };

#pragma pack(push, 2)
    struct FHalfVector4
    {
        FFloat16 X;
        FFloat16 Y;
        FFloat16 Z;
        FFloat16 W;

        FHalfVector4() = default;
        FHalfVector4(FFloat16 x, FFloat16 y, FFloat16 z, FFloat16 w) : X(x), Y(y), Z(z), W(w) {}

        FVector ToFVector() const { return FVector(X.ToFloat(), Y.ToFloat(), Z.ToFloat()) * W.ToFloat(); }
    };

    struct FHalfVectorScaled
    {
        FFloat16 X;
        FFloat16 Y;
        FFloat16 Z;
        FFloat16 W;

        FHalfVectorScaled() = default;
        FHalfVectorScaled(FFloat16 x, FFloat16 y, FFloat16 z, FFloat16 w) : X(x), Y(y), Z(z), W(w) {}

        FVector ToFVector() const
        {
            // Compared as a float, not as an encoding, so negative zero counts as zero exactly as in C#.
            const float w = W.ToFloat();
            const float wf = w == 0.0f ? 1.0f : w;
            return FVector(X.ToFloat(), Y.ToFloat(), Z.ToFloat()) * wf;
        }
    };
#pragma pack(pop)

    inline FHalfVectorScaled FHalfVector::ToScaled() const { return FHalfVectorScaled(X, Y, Z, FFloat16()); }
}
