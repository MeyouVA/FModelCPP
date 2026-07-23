// Cross-type definitions for FQuat (declared in FQuat.h) plus the FQuatRotationTranslationMatrix ctor.
// These need FMatrix / FRotator to be complete.
#include "FQuat.h"

#include <cmath>

#include "FRotator.h"
#include "Matrix.h"
#include "QuatRotationTranslationMatrix.h"
#include "../../../../Utils/MathUtils.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    namespace
    {
        constexpr int matrixNxt[3] = {1, 2, 0};
    }

    FQuat::FQuat(const FMatrix& m)
    {
        // If Matrix is NULL, return Identity quaternion. If any of them is 0, you won't be able to construct
        // rotation; if you convert to matrix from 0 scale and convert back, you'll lose rotation. Don't do that.
        if (m.GetScaledAxis(EAxis::X).IsNearlyZero() || m.GetScaledAxis(EAxis::Y).IsNearlyZero() || m.GetScaledAxis(EAxis::Z).IsNearlyZero())
        {
            *this = Identity;
            return;
        }

        float s;

        // Check diagonal (trace)
        const float tr = m.M00 + m.M11 + m.M22;

        if (tr > 0.0f)
        {
            const float invS = 1.0f / std::sqrt(tr + 1.0f);
            W = 0.5f * (1.0f / invS);
            s = 0.5f * invS;

            X = (m.M12 - m.M21) * s;
            Y = (m.M20 - m.M02) * s;
            Z = (m.M01 - m.M10) * s;
        }
        else
        {
            // diagonal is negative
            int i = 0;

            if (m.M11 > m.M00)
                i = 1;

            if (m.M22 > m[4 * i + i])
                i = 2;

            const int j = matrixNxt[i];
            const int k = matrixNxt[j];

            s = m[4 * i + i] - m[4 * j + j] - m[4 * k + k] + 1.0f;

            const float invS = 1.0f / std::sqrt(s);

            float qt[4];
            qt[i] = 0.5f * (1.0f / invS);

            s = 0.5f * invS;

            qt[3] = (m[4 * j + k] - m[4 * k + j]) * s;
            qt[j] = (m[4 * i + j] + m[4 * j + i]) * s;
            qt[k] = (m[4 * i + k] + m[4 * k + i]) * s;

            X = qt[0];
            Y = qt[1];
            Z = qt[2];
            W = qt[3];
        }
    }

    FQuat::FQuat(const FRotator& rotator) { *this = rotator.Quaternion(); }

    FRotator FQuat::Rotator() const
    {
        const float singularityTest = Z * X - W * Y;
        const float yawY = 2.0f * (W * Z + X * Y);
        const float yawX = 1.0f - 2.0f * (Y * Y + Z * Z);

        // reference:
        // http://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
        // http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/
        // This threshold was found from experience; the sites above recommend different values.
        constexpr float SINGULARITY_THRESHOLD = 0.4999995f;
        const float RAD_TO_DEG = 180.0f / CUE4Parse::Utils::MathConstants::PI_F;
        FRotator rotatorFromQuat;

        if (singularityTest < -SINGULARITY_THRESHOLD)
        {
            rotatorFromQuat.Pitch = -90.0f;
            rotatorFromQuat.Yaw = std::atan2(yawY, yawX) * RAD_TO_DEG;
            rotatorFromQuat.Roll = FRotator::NormalizeAxis(-rotatorFromQuat.Yaw - (2.0f * std::atan2(X, W) * RAD_TO_DEG));
        }
        else if (singularityTest > SINGULARITY_THRESHOLD)
        {
            rotatorFromQuat.Pitch = 90.0f;
            rotatorFromQuat.Yaw = std::atan2(yawY, yawX) * RAD_TO_DEG;
            rotatorFromQuat.Roll = FRotator::NormalizeAxis(rotatorFromQuat.Yaw - (2.0f * std::atan2(X, W) * RAD_TO_DEG));
        }
        else
        {
            rotatorFromQuat.Pitch = std::asin(2.0f * singularityTest) * RAD_TO_DEG;
            rotatorFromQuat.Yaw = std::atan2(yawY, yawX) * RAD_TO_DEG;
            rotatorFromQuat.Roll = std::atan2(-2.0f * (W * X + Y * Z), 1.0f - 2.0f * (X * X + Y * Y)) * RAD_TO_DEG;
        }

        return rotatorFromQuat;
    }

    // Ported from QuatRotationTranslationMatrix.cs.
    FQuatRotationTranslationMatrix::FQuatRotationTranslationMatrix(const FQuat& q, const FVector& origin)
    {
        const float x2 = q.X + q.X;  const float y2 = q.Y + q.Y;  const float z2 = q.Z + q.Z;
        const float xx = q.X * x2;   const float xy = q.X * y2;   const float xz = q.X * z2;
        const float yy = q.Y * y2;   const float yz = q.Y * z2;   const float zz = q.Z * z2;
        const float wx = q.W * x2;   const float wy = q.W * y2;   const float wz = q.W * z2;

        M00 = 1.0f - (yy + zz);  M10 = xy - wz;           M20 = xz + wy;           M30 = origin.X;
        M01 = xy + wz;           M11 = 1.0f - (xx + zz);  M21 = yz - wx;           M31 = origin.Y;
        M02 = xz - wy;           M12 = yz + wx;           M22 = 1.0f - (xx + yy);  M32 = origin.Z;
        M03 = 0.0f;              M13 = 0.0f;              M23 = 0.0f;              M33 = 1.0f;
    }
}
