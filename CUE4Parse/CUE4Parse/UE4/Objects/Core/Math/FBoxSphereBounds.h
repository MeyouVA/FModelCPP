// Ported from CUE4Parse/UE4/Objects/Core/Math/FBoxSphereBounds.cs.
// C# does the TransformBy arithmetic in System.Numerics.Vector3 and converts back at the end; there is no such
// type here, so FVector carries it throughout. The operations used (component-wise multiply, Abs, Max, Dot) are
// element-wise and identical in both, so the results match bit for bit.
#pragma once

#include <algorithm>
#include <cmath>
#include <string>

#include "FBox.h"
#include "FSphere.h"
#include "FTransform.h"
#include "FVector.h"
#include "Matrix.h"
#include "../../../Readers/FArchive.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    using CUE4Parse::UE4::Readers::FArchive;

    /// Structure for a combined axis aligned bounding box and bounding sphere with the same origin. (28 bytes).
    struct FBoxSphereBounds
    {
        /** Holds the origin of the bounding box and sphere. */
        FVector Origin;
        /** Holds the extent of the bounding box. */
        FVector BoxExtent;
        /** Holds the radius of the bounding sphere. */
        float SphereRadius = 0.0f;

        FBoxSphereBounds() = default;
        explicit FBoxSphereBounds(FArchive& Ar) : Origin(Ar), BoxExtent(Ar), SphereRadius(Ar.ReadFReal()) {}
        FBoxSphereBounds(FVector origin, FVector boxExtent, float sphereRadius)
            : Origin(origin), BoxExtent(boxExtent), SphereRadius(sphereRadius) {}

        FBoxSphereBounds(const FBox& box, const FSphere& sphere)
        {
            box.GetCenterAndExtents(Origin, BoxExtent);
            SphereRadius = std::min(BoxExtent.Size(), (sphere.Center - Origin).Size() + sphere.W);
        }

        explicit FBoxSphereBounds(const FBox& box)
        {
            box.GetCenterAndExtents(Origin, BoxExtent);
            SphereRadius = BoxExtent.Size();
        }

        explicit FBoxSphereBounds(const FSphere& sphere)
            : Origin(sphere.Center), BoxExtent(sphere.W), SphereRadius(sphere.W) {}

        FBox GetBox() const { return FBox(Origin - BoxExtent, Origin + BoxExtent); }

        FBoxSphereBounds TransformBy(const FMatrix& m) const
        {
            FBoxSphereBounds result;

            const FVector vecOrigin = Origin;
            const FVector vecExtent = BoxExtent;

            const FVector m0(m.M00, m.M01, m.M02);
            const FVector m1(m.M10, m.M11, m.M12);
            const FVector m2(m.M20, m.M21, m.M22);
            const FVector m3(m.M30, m.M31, m.M32);

            const FVector newOrigin = FVector(vecOrigin.X) * m0 +
                                      FVector(vecOrigin.Y) * m1 +
                                      FVector(vecOrigin.Z) * m2 +
                                      m3;

            const FVector newExtent = (FVector(vecExtent.X) * m0).Abs() +
                                      (FVector(vecExtent.Y) * m1).Abs() +
                                      (FVector(vecExtent.Z) * m2).Abs();

            result.BoxExtent = newExtent;
            result.Origin = newOrigin;

            FVector maxRadius = m0 * m0 + m1 * m1 + m2 * m2;
            maxRadius = ComponentMax(ComponentMax(maxRadius, FVector(maxRadius.Y)), FVector(maxRadius.Z));
            result.SphereRadius = std::sqrt(maxRadius.X) * SphereRadius;

            // For non-uniform scaling, computing sphere radius from a box results in a smaller sphere.
            const float boxExtentMagnitude = std::sqrt(FVector::DotProduct(newExtent, newExtent));
            result.SphereRadius = std::min(result.SphereRadius, boxExtentMagnitude);

            return result;
        }

        FBoxSphereBounds TransformBy(const FTransform& m) const { return TransformBy(m.ToMatrixWithScale()); }

        std::string ToString() const
        {
            return "Origin=(" + Origin.ToString() + "), BoxExtent=(" + BoxExtent.ToString() +
                   "), SphereRadius=" + std::to_string(SphereRadius);
        }

    private:
        // System.Numerics' Vector3.Max — component-wise, with no FVector equivalent in the port.
        static FVector ComponentMax(FVector a, FVector b)
        {
            return {std::max(a.X, b.X), std::max(a.Y, b.Y), std::max(a.Z, b.Z)};
        }
    };
}
