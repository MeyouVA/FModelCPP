// Ported from the EForceInit enum declared alongside FQuat in CUE4Parse/UE4/Objects/Core/Math/FQuat.cs.
// Broken out into its own header because FRotator, FQuat and FTransform all take it as an init selector.
#pragma once

namespace CUE4Parse::UE4::Objects::Core::Math
{
    enum class EForceInit
    {
        ForceInit,
        ForceInitToZero
    };
}
