// Ported from CUE4Parse/UE4/Objects/Core/Math/FTransform.cs — the QST (quaternion/scale/translation) transform.
//
// Every type FTransform touches (FQuat, FVector, FMatrix, FRotator) is complete by the time this header is
// included, so unlike the rest of the geometry cluster it needs no out-of-line translation unit.
//
// Deferred:
//   - the [StructFallback] FTransform(FStructFallback) ctor — the Math layer deliberately does not depend on
//     the Assets/Objects layer; it will be wired from the property side.
//   - TTransform<T> — needs TIntVector3/TIntVector4, which are not ported.
//   - Clone() (C# ICloneable) — a C++ copy is already a clone.
#pragma once

#include <cmath>
#include <stdexcept>
#include <string>

#include "FQuat.h"
#include "FRotator.h"
#include "FVector.h"
#include "Matrix.h"
#include "EForceInit.h"
#include "UnrealMathUtility.h"
#include "../../../Readers/FArchive.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    using CUE4Parse::UE4::Readers::FArchive;

    struct FTransform
    {
        static const FTransform Identity;

        FQuat Rotation;
        FVector Translation;
        FVector Scale3D;

        bool IsRotationNormalized() const { return Rotation.IsNormalized(); }

        FTransform() : Rotation(0.0f, 0.0f, 0.0f, 1.0f), Translation(0.0f), Scale3D(FVector::OneVector) {}
        explicit FTransform(EForceInit) : FTransform() {}

        explicit FTransform(FArchive& Ar) : Rotation(Ar), Translation(Ar), Scale3D(Ar) {}

        explicit FTransform(const FVector& translation)
            : Rotation(FQuat::Identity), Translation(translation), Scale3D(FVector::OneVector) {}

        explicit FTransform(const FQuat& rotation)
            : Rotation(rotation), Translation(FVector::ZeroVector), Scale3D(FVector::OneVector) {}

        explicit FTransform(const FRotator& rotation)
            : Rotation(FQuat(rotation)), Translation(FVector::ZeroVector), Scale3D(FVector::OneVector) {}

        FTransform(const FQuat& rotation, const FVector& translation, const FVector& scale3D)
            : Rotation(rotation), Translation(translation), Scale3D(scale3D) {}

        FTransform(const FRotator& rotation, const FVector& translation, const FVector& scale3D)
            : Rotation(FQuat(rotation)), Translation(translation), Scale3D(scale3D) {}

        FTransform(const FVector& inX, const FVector& inY, const FVector& inZ, const FVector& inW)
        {
            SetFromMatrix(FMatrix(inX, inY, inZ, inW));
        }

        void SetFromMatrix(const FMatrix& inMatrix)
        {
            FMatrix m(inMatrix);

            // Get the 3D scale from the matrix.
            Scale3D = m.ExtractScaling();

            // If there is negative scaling going on, we handle that here.
            if (inMatrix.Determinant() < 0.0f)
            {
                // Assume it is along X and modify transform accordingly.
                Scale3D.X *= -1.0f;
                m.SetAxis(0, -m.GetScaledAxis(EAxis::X));
            }

            Rotation = m.ToQuat();
            Translation = inMatrix.GetOrigin();

            // Normalize rotation.
            Rotation.Normalize();
        }

        void SetRotation(const FQuat& rotation) { Rotation = rotation; }
        void SetLocation(const FVector& origin) { Translation = origin; }
        void SetScale3D(const FVector& scale) { Scale3D = scale; }

        FRotator Rotator() const { return Rotation.Rotator(); }

        float GetDeterminant() const { return Scale3D.X * Scale3D.Y * Scale3D.Z; }

        bool Equals(const FTransform& other, float tolerance = UnrealMath::KindaSmallNumber) const
        {
            return Rotation.Equals(other.Rotation, tolerance) &&
                Translation.Equals(other.Translation, tolerance) &&
                Scale3D.Equals(other.Scale3D, tolerance);
        }

        bool ContainsNaN() const { return Translation.ContainsNaN() || Rotation.ContainsNaN() || Scale3D.ContainsNaN(); }

        static bool AnyHasNegativeScale(const FVector& scale3D, const FVector& otherScale3D)
        {
            return scale3D.X < 0 || scale3D.Y < 0 || scale3D.Z < 0 ||
                otherScale3D.X < 0 || otherScale3D.Y < 0 || otherScale3D.Z < 0;
        }

        void ScaleTranslation(const FVector& scale3D) { Translation = Translation * scale3D; }
        void ScaleTranslation(float scale) { Translation = Translation * scale; }

        void RemoveScaling(float /*tolerance*/ = UnrealMath::SmallNumber)
        {
            Scale3D = FVector(1.0f, 1.0f, 1.0f);
            Rotation.Normalize();
        }

        float GetMaximumAxisScale() const { return Scale3D.AbsMax(); }
        float GetMinimumAxisScale() const { return Scale3D.AbsMin(); }

        void CopyTranslation(const FTransform& other) { Translation = other.Translation; }
        void CopyRotation(const FTransform& other) { Rotation = other.Rotation; }
        void CopyScale3D(const FTransform& other) { Scale3D = other.Scale3D; }

        FTransform Inverse() const
        {
            const FQuat invRotation = Rotation.Inverse();
            // this used to cause NaN if Scale contained 0
            const FVector invScale3D = GetSafeScaleReciprocal(Scale3D);
            const FVector invTranslation = invRotation * (invScale3D * -Translation);

            return FTransform(invRotation, invTranslation, invScale3D);
        }

        FTransform GetRelativeTransform(const FTransform& other) const
        {
            // A * B(-1) = VQS(B)(-1) (VQS (A))
            //   Scale = S(A)/S(B), Rotation = Q(B)(-1) * Q(A),
            //   Translation = 1/S(B) *[Q(B)(-1)*(T(A)-T(B))*Q(B)]   where A = this, B = Other
            FTransform result{EForceInit::ForceInit};

            if (AnyHasNegativeScale(Scale3D, other.Scale3D))
            {
                // @note, if you have 0 scale with negative, you're going to lose rotation as it can't convert back to quat
                GetRelativeTransformUsingMatrixWithScale(result, other);
            }
            else
            {
                const FVector safeRecipScale3D = GetSafeScaleReciprocal(other.Scale3D, UnrealMath::SmallNumber);
                result.Scale3D = Scale3D * safeRecipScale3D;

                if (!other.Rotation.IsNormalized())
                    return Identity;

                const FQuat inverse = other.Rotation.Inverse();
                result.Rotation = inverse * Rotation;

                result.Translation = (inverse * (Translation - other.Translation)) * safeRecipScale3D;
            }

            return result;
        }

        static FVector SubstractTranslations(const FTransform& a, const FTransform& b) { return a.Translation - b.Translation; }

        void GetRelativeTransformUsingMatrixWithScale(FTransform& outTransform, const FTransform& relative) const
        {
            // the goal of using M is to get the correct orientation, but for translation we still need scale
            const FMatrix am = ToMatrixWithScale();
            const FMatrix bm = ToMatrixWithScale();
            const FVector safeRecipScale3D = GetSafeScaleReciprocal(relative.Scale3D, UnrealMath::SmallNumber);
            const FVector desiredScale3D = Scale3D * safeRecipScale3D;
            ConstructTransformFromMatrixWithDesiredScale(am, bm.InverseFast(), desiredScale3D, outTransform);
        }

        static void ConstructTransformFromMatrixWithDesiredScale(const FMatrix& aMatrix, const FMatrix& bMatrix, const FVector& desiredScale, FTransform& outTransform)
        {
            FMatrix m = aMatrix * bMatrix;
            m.RemoveScaling();

            // apply negative scale back to axes
            const FVector signedScale = desiredScale.GetSignVector();

            m.SetAxis(0, signedScale.X * m.GetScaledAxis(EAxis::X));
            m.SetAxis(1, signedScale.Y * m.GetScaledAxis(EAxis::Y));
            m.SetAxis(2, signedScale.Z * m.GetScaledAxis(EAxis::Z));

            // @note: if you have negative with 0 scale, this will return rotation that is identity
            FQuat rotation(m);
            rotation.Normalize();

            outTransform.Scale3D = desiredScale;
            outTransform.Rotation = rotation;
            outTransform.Translation = m.GetOrigin();
        }

        /** Convert this Transform to a transformation matrix with scaling. */
        FMatrix ToMatrixWithScale() const
        {
            FMatrix outMatrix;

            outMatrix.M30 = Translation.X;
            outMatrix.M31 = Translation.Y;
            outMatrix.M32 = Translation.Z;

            const float x2 = Rotation.X + Rotation.X;
            const float y2 = Rotation.Y + Rotation.Y;
            const float z2 = Rotation.Z + Rotation.Z;
            {
                const float xx2 = Rotation.X * x2;
                const float yy2 = Rotation.Y * y2;
                const float zz2 = Rotation.Z * z2;

                outMatrix.M00 = (1.0f - (yy2 + zz2)) * Scale3D.X;
                outMatrix.M11 = (1.0f - (xx2 + zz2)) * Scale3D.Y;
                outMatrix.M22 = (1.0f - (xx2 + yy2)) * Scale3D.Z;
            }
            {
                const float yz2 = Rotation.Y * z2;
                const float wx2 = Rotation.W * x2;

                outMatrix.M21 = (yz2 - wx2) * Scale3D.Z;
                outMatrix.M12 = (yz2 + wx2) * Scale3D.Y;
            }
            {
                const float xy2 = Rotation.X * y2;
                const float wz2 = Rotation.W * z2;

                outMatrix.M10 = (xy2 - wz2) * Scale3D.Y;
                outMatrix.M01 = (xy2 + wz2) * Scale3D.X;
            }
            {
                const float xz2 = Rotation.X * z2;
                const float wy2 = Rotation.W * y2;

                outMatrix.M20 = (xz2 + wy2) * Scale3D.Z;
                outMatrix.M02 = (xz2 - wy2) * Scale3D.X;
            }

            outMatrix.M03 = 0.0f;
            outMatrix.M13 = 0.0f;
            outMatrix.M23 = 0.0f;
            outMatrix.M33 = 1.0f;

            return outMatrix;
        }

        // Mathematically a 0 scale should give an infinite reciprocal, but in practice that just produces
        // gigantic meshes and sequential NaNs, so it is hardcoded to 0.
        static FVector GetSafeScaleReciprocal(const FVector& scale, float tolerance = UnrealMath::SmallNumber)
        {
            FVector safeReciprocalScale;
            safeReciprocalScale.X = std::fabs(scale.X) <= tolerance ? 0.0f : 1.0f / scale.X;
            safeReciprocalScale.Y = std::fabs(scale.Y) <= tolerance ? 0.0f : 1.0f / scale.Y;
            safeReciprocalScale.Z = std::fabs(scale.Z) <= tolerance ? 0.0f : 1.0f / scale.Z;
            return safeReciprocalScale;
        }

        /** Returns the multiplied transform of 2 FTransforms. */
        friend FTransform operator*(const FTransform& a, const FTransform& b)
        {
            if (!a.IsRotationNormalized()) throw std::invalid_argument("Rotation a must be normalized for multiplication");
            if (!b.IsRotationNormalized()) throw std::invalid_argument("Rotation b must be normalized for multiplication");

            //  Q(AxB) = Q(B)*Q(A); S(AxB) = S(A)*S(B); T(AxB) = Q(B)*S(B)*T(A)*-Q(B) + T(B)
            FTransform result{EForceInit::ForceInit};
            if (AnyHasNegativeScale(a.Scale3D, b.Scale3D))
            {
                // @note, if you have 0 scale with negative, you're going to lose rotation as it can't convert back to quat
                MultiplyUsingMatrixWithScale(result, a, b);
            }
            else
            {
                result.Rotation = b.Rotation * a.Rotation;
                result.Scale3D = b.Scale3D * a.Scale3D;
                result.Translation = b.Rotation * (b.Scale3D * a.Translation) + b.Translation;
            }

            return result;
        }

        static void MultiplyUsingMatrixWithScale(FTransform& outTransform, const FTransform& a, const FTransform& b)
        {
            ConstructTransformFromMatrixWithDesiredScale(a.ToMatrixWithScale(), b.ToMatrixWithScale(), a.Scale3D * b.Scale3D, outTransform);
        }

        FVector TransformPosition(const FVector& v) const { return Rotation.RotateVector(Scale3D * v) + Translation; }
        FVector TransformPositionNoScale(const FVector& v) const { return Rotation.RotateVector(v) + Translation; }
        FVector InverseTransformPosition(const FVector& v) const { return Rotation.UnrotateVector(v - Translation) * GetSafeScaleReciprocal(Scale3D); }
        FVector InverseTransformPositionNoScale(const FVector& v) const { return Rotation.UnrotateVector(v - Translation); }
        FVector TransformVector(const FVector& v) const { return Rotation.RotateVector(Scale3D * v); }
        FVector TransformVectorNoScale(const FVector& v) const { return Rotation.RotateVector(v); }

        FQuat TransformRotation(const FQuat& q) const { return Rotation * q; }
        FQuat InverseTransformRotation(const FQuat& q) const { return Rotation.Inverse() * q; }

        FTransform GetScaled(float scale) const
        {
            FTransform a = *this;
            a.Scale3D = a.Scale3D * scale;
            return a;
        }

        FTransform GetScaled(const FVector& scale) const
        {
            FTransform a = *this;
            a.Scale3D = a.Scale3D * scale;
            return a;
        }

        FVector GetScaledAxis(EAxis axis) const
        {
            switch (axis)
            {
                case EAxis::X: return TransformVector(FVector(1.0f, 0.0f, 0.0f));
                case EAxis::Y: return TransformVector(FVector(0.0f, 1.0f, 0.0f));
                default: return TransformVector(FVector(0.0f, 0.0f, 1.0f));
            }
        }

        FVector GetUnitAxis(EAxis axis) const
        {
            switch (axis)
            {
                case EAxis::X: return TransformVectorNoScale(FVector(1.0f, 0.0f, 0.0f));
                case EAxis::Y: return TransformVectorNoScale(FVector(0.0f, 1.0f, 0.0f));
                default: return TransformVectorNoScale(FVector(0.0f, 0.0f, 1.0f));
            }
        }

        std::string ToString() const
        {
            return "{T:" + Translation.ToString() + " R:" + Rotation.ToString() + " S:" + Scale3D.ToString() + "}";
        }
    };

    inline const FTransform FTransform::Identity{FQuat::Identity, FVector::ZeroVector, FVector::OneVector};
}
