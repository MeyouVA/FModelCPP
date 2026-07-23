// Cross-type definitions for FMatrix (declared in Matrix.h) plus the FRotationTranslationMatrix ctor.
// These live here rather than in the header because they need FRotator / FQuat / FPlane to be complete.
#include "Matrix.h"

#include <cmath>

#include "FPlane.h"
#include "FQuat.h"
#include "FRotator.h"
#include "RotationMatrix.h"
#include "RotationTranslationMatrix.h"
#include "../../../../Utils/MathUtils.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    FRotator FMatrix::Rotator() const
    {
        const FVector xAxis = GetScaledAxis(EAxis::X);
        const FVector yAxis = GetScaledAxis(EAxis::Y);
        const FVector zAxis = GetScaledAxis(EAxis::Z);

        FRotator rotator(
            std::atan2(xAxis.Z, std::sqrt(xAxis.X * xAxis.X + xAxis.Y * xAxis.Y)) * 180.0f / CUE4Parse::Utils::MathConstants::PI_F,
            std::atan2(xAxis.Y, xAxis.X) * 180.0f / CUE4Parse::Utils::MathConstants::PI_F,
            0.0f);

        const FVector syAxis = FRotationMatrix(rotator).GetScaledAxis(EAxis::Y);
        rotator.Roll = std::atan2(zAxis | syAxis, yAxis | syAxis) * 180.0f / CUE4Parse::Utils::MathConstants::PI_F;

        return rotator;
    }

    FQuat FMatrix::ToQuat() const { return FQuat(*this); }

    bool FMatrix::MakeFrustumPlane(float a, float b, float c, float d, FPlane& plane) const
    {
        const float lengthSquared = a * a + b * b + c * c;
        if (lengthSquared > 0.00001f * 0.00001f)
        {
            const float invLength = CUE4Parse::Utils::InvSqrt(lengthSquared);
            plane = FPlane(-a * invLength, -b * invLength, -c * invLength, d * invLength);
            return true;
        }

        plane = FPlane();
        return false;
    }

    bool FMatrix::GetFrustumNearPlane(FPlane& plane) const { return MakeFrustumPlane(M03 - M02, M13 - M12, M23 - M22, M33 - M32, plane); }
    bool FMatrix::GetFrustumFarPlane(FPlane& plane) const { return MakeFrustumPlane(M02, M12, M22, M32, plane); }
    bool FMatrix::GetFrustumLeftPlane(FPlane& plane) const { return MakeFrustumPlane(M03 + M00, M13 + M10, M23 + M20, M33 + M30, plane); }
    bool FMatrix::GetFrustumRightPlane(FPlane& plane) const { return MakeFrustumPlane(M03 - M00, M13 - M10, M23 - M20, M33 - M30, plane); }
    bool FMatrix::GetFrustumTopPlane(FPlane& plane) const { return MakeFrustumPlane(M03 - M01, M13 - M11, M23 - M21, M33 - M31, plane); }
    bool FMatrix::GetFrustumBottomPlane(FPlane& plane) const { return MakeFrustumPlane(M03 + M01, M13 + M11, M23 + M21, M33 + M31, plane); }

    // Ported from RotationTranslationMatrix.cs.
    FRotationTranslationMatrix::FRotationTranslationMatrix(const FRotator& rot, const FVector& origin)
    {
        const float p = rot.Pitch / 180.0f * CUE4Parse::Utils::MathConstants::PI_F;
        const float y = rot.Yaw / 180.0f * CUE4Parse::Utils::MathConstants::PI_F;
        const float r = rot.Roll / 180.0f * CUE4Parse::Utils::MathConstants::PI_F;
        const float sP = std::sin(p);
        const float sY = std::sin(y);
        const float sR = std::sin(r);
        const float cP = std::cos(p);
        const float cY = std::cos(y);
        const float cR = std::cos(r);

        M00 = cP * cY;
        M01 = cP * sY;
        M02 = sP;
        M03 = 0.0f;

        M10 = sR * sP * cY - cR * sY;
        M11 = sR * sP * sY + cR * cY;
        M12 = -sR * cP;
        M13 = 0.0f;

        M20 = -(cR * sP * cY + sR * sY);
        M21 = cY * sR - cR * sP * sY;
        M22 = cR * cP;
        M23 = 0.0f;

        M30 = origin.X;
        M31 = origin.Y;
        M32 = origin.Z;
        M33 = 1.0f;
    }
}
