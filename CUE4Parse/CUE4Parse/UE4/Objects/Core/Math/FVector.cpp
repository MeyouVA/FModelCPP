// Ported from CUE4Parse/UE4/Objects/Core/Math/{FVector,FVector4}.cs.
// Holds the FVector4 <-> FVector cross-conversions (declared in FVector4.h, defined here where both
// types are complete) and gives the header-only vector layer a translation unit in the library.
#include "FVector.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    FVector4::FVector4(const FVector& v, float w) : X(v.X), Y(v.Y), Z(v.Z), W(w) {}

    FVector4::operator FVector() const { return FVector(X, Y, Z); }
}
