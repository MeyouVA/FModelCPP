// Ported from CUE4Parse/UE4/Objects/Engine/Curves/UCurveFloat.cs
// `public class UCurveFloat : UCurveBase;` -- an empty subclass. The single FRichCurve of a curve-float asset is
// carried as an ordinary tagged StructProperty ("FloatCurve") on the object, read by the base UObject property
// path; C# adds no dedicated members or Deserialize override, so neither does this port.
#pragma once

#include "UCurveBase.h"

namespace CUE4Parse::UE4::Objects::Engine::Curves
{
    class UCurveFloat : public UCurveBase
    {
    };
}
