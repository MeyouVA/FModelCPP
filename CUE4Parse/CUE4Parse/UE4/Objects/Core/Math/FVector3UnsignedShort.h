// Ported from CUE4Parse/UE4/Objects/Core/Math/FVector3UnsignedShort.cs.
// Four halves on the wire, three of which become the vector — W is not sure about, seems to be always 0x00 0x3c
// (i.e. 1.0), and C# drops it too.
#pragma once

#include "FFloat16.h"
#include "FVector.h"
#include "../../../IUStruct.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    struct FVector3UnsignedShort : public UE4::IUStruct
    {
        FFloat16 X;
        FFloat16 Y;
        FFloat16 Z;
        FFloat16 W;

        FVector3UnsignedShort() = default;
        FVector3UnsignedShort(FFloat16 x, FFloat16 y, FFloat16 z, FFloat16 w) : X(x), Y(y), Z(z), W(w) {}

        FVector ToFVector() const { return {X.ToFloat(), Y.ToFloat(), Z.ToFloat()}; }
    };
}
