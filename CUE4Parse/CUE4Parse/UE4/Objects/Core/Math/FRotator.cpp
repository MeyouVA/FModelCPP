// Cross-type definitions for FRotator (declared in FRotator.h): the vector rotation helpers need
// FRotationMatrix, and Quaternion() needs FQuat.
#include "FRotator.h"

#include <cmath>

#include "FQuat.h"
#include "RotationMatrix.h"
#include "../../../../Utils/MathUtils.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    FVector FRotator::RotateVector(const FVector& v) const
    {
        return static_cast<FVector>(FRotationMatrix(*this).TransformVector(v));
    }

    FVector FRotator::UnrotateVector(const FVector& v) const
    {
        return static_cast<FVector>(FRotationMatrix(*this).GetTransposed().TransformVector(v));
    }

    FQuat FRotator::Quaternion() const
    {
        // PLATFORM_ENABLE_VECTORINTRINSICS
        constexpr float DEG_TO_RAD = CUE4Parse::Utils::MathConstants::PI_F / 180.0f;
        constexpr float DIVIDE_BY_2 = DEG_TO_RAD / 2.0f;

        const float sp = std::sin(Pitch * DIVIDE_BY_2);
        const float cp = std::cos(Pitch * DIVIDE_BY_2);
        const float sy = std::sin(Yaw * DIVIDE_BY_2);
        const float cy = std::cos(Yaw * DIVIDE_BY_2);
        const float sr = std::sin(Roll * DIVIDE_BY_2);
        const float cr = std::cos(Roll * DIVIDE_BY_2);

        FQuat rotationQuat;
        rotationQuat.X = cr * sp * sy - sr * cp * cy;
        rotationQuat.Y = -cr * sp * cy - sr * cp * sy;
        rotationQuat.Z = cr * cp * sy - sr * sp * cy;
        rotationQuat.W = cr * cp * cy + sr * sp * sy;

        return rotationQuat;
    }
}
