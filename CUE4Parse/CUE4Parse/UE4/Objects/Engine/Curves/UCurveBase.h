// Ported from CUE4Parse/UE4/Objects/Engine/Curves/UCurveBase.cs
// The abstract base of the curve-asset exports (UCurveFloat / UCurveVector / UCurveLinearColor). In C# it is
// `public abstract class UCurveBase : UObject;` with no body of its own; the concrete subclasses add the
// FRichCurve array(s) and pull them out of the object's own tagged properties.
//
// Note: C# marks this abstract, but it adds no pure-virtual members over UObject; this port keeps it a plain
// UObject subclass (it is never registered nor instantiated directly, so abstractness is moot).
#pragma once

#include "../../../Assets/Exports/UObject.h"

namespace CUE4Parse::UE4::Objects::Engine::Curves
{
    class UCurveBase : public CUE4Parse::UE4::Assets::Exports::UObject
    {
    };
}
