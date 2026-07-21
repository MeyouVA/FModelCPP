// Ported from CUE4Parse/UE4/Objects/Engine/Curves/UCurveVector.cs
// A curve-vector asset: three FRichCurves (X/Y/Z), each stored as a top-level StructProperty on the object.
//
// Deliberate differences from C#:
//   * FloatCurves is a std::array<FRichCurve, 3> of default-constructed (empty) curves rather than a C#
//     FRichCurve[3] of nulls; an unassigned slot is therefore an empty curve (0 keys) instead of null. The
//     positional assignment (FloatCurves[i] for the i-th property) mirrors C# exactly, but is bounded to the
//     array size: C# would throw IndexOutOfRange if Properties.Count > 3, whereas here extra properties are
//     ignored (real assets carry exactly the 3 curve structs, in order).
//   * WriteJson is omitted.
#pragma once

#include <array>
#include <cstdint>

#include "RichCurve.h"
#include "UCurveBase.h"

namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }

namespace CUE4Parse::UE4::Objects::Engine::Curves
{
    class UCurveVector : public UCurveBase
    {
    public:
        std::array<FRichCurve, 3> FloatCurves;

        void Deserialize(CUE4Parse::UE4::Assets::Readers::FAssetArchive& Ar, int64_t validPos) override;
    };
}
