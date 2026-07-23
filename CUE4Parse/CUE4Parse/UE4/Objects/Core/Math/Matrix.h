// Ported from CUE4Parse/UE4/Objects/Core/Math/Matrix.cs — the 4x4 row-major float matrix (FMatrix),
// its EAxis enum and the packed FMatrix3x4. C#'s FMatrix is a reference type; here it is a value type
// (every use in the source copies/mutates locals, so value semantics are faithful).
//
// The cross-type members (Rotator/ToQuat, which touch FRotator/FQuat, and the frustum-plane helpers,
// which touch FPlane) are declared here and defined in Matrix.cpp where those types are complete.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>

#include "FVector.h"
#include "FVector4.h"
#include "UnrealMathUtility.h"
#include "../../../Readers/FArchive.h"
#include "../../../Versions/ObjectVersion.h"
#include "../../../../Utils/MathUtils.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    using CUE4Parse::UE4::Readers::FArchive;
    using CUE4Parse::UE4::Versions::EUnrealEngineObjectUE5Version;

    struct FRotator; // FRotator.h
    struct FQuat;    // FQuat.h
    struct FPlane;   // FPlane.h

    // Generic axis enum (mirrored for property use in Object.h).
    enum class EAxis
    {
        None,
        X,
        Y,
        Z
    };

    // A 3x4 block of floats (readonly in C#).
    struct FMatrix3x4
    {
        float M00 = 0, M01 = 0, M02 = 0;
        float M10 = 0, M11 = 0, M12 = 0;
        float M20 = 0, M21 = 0, M22 = 0;
        float M30 = 0, M31 = 0, M32 = 0;
    };

    class FMatrix
    {
    public:
        static FMatrix Identity()
        {
            return FMatrix(
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f);
        }

        float M00 = 0, M01 = 0, M02 = 0, M03 = 0;
        float M10 = 0, M11 = 0, M12 = 0, M13 = 0;
        float M20 = 0, M21 = 0, M22 = 0, M23 = 0;
        float M30 = 0, M31 = 0, M32 = 0, M33 = 0;

        FMatrix() = default;

        FMatrix(
            float m00, float m01, float m02, float m03,
            float m10, float m11, float m12, float m13,
            float m20, float m21, float m22, float m23,
            float m30, float m31, float m32, float m33)
            : M00(m00), M01(m01), M02(m02), M03(m03),
              M10(m10), M11(m11), M12(m12), M13(m13),
              M20(m20), M21(m21), M22(m22), M23(m23),
              M30(m30), M31(m31), M32(m32), M33(m33) {}

        FMatrix(const FVector& inX, const FVector& inY, const FVector& inZ, const FVector& inW)
        {
            M00 = inX.X; M01 = inX.Y; M02 = inX.Z; M03 = 0.0f;
            M10 = inY.X; M11 = inY.Y; M12 = inY.Z; M13 = 0.0f;
            M20 = inZ.X; M21 = inZ.Y; M22 = inZ.Z; M23 = 0.0f;
            M30 = inW.X; M31 = inW.Y; M32 = inW.Z; M33 = 1.0f;
        }

        explicit FMatrix(FArchive& Ar) : FMatrix(Ar, Ar.Ver() >= EUnrealEngineObjectUE5Version::LARGE_WORLD_COORDINATES) {}

        FMatrix(FArchive& Ar, bool readDouble)
        {
            if (readDouble)
            {
                M00 = static_cast<float>(Ar.Read<double>()); M01 = static_cast<float>(Ar.Read<double>());
                M02 = static_cast<float>(Ar.Read<double>()); M03 = static_cast<float>(Ar.Read<double>());
                M10 = static_cast<float>(Ar.Read<double>()); M11 = static_cast<float>(Ar.Read<double>());
                M12 = static_cast<float>(Ar.Read<double>()); M13 = static_cast<float>(Ar.Read<double>());
                M20 = static_cast<float>(Ar.Read<double>()); M21 = static_cast<float>(Ar.Read<double>());
                M22 = static_cast<float>(Ar.Read<double>()); M23 = static_cast<float>(Ar.Read<double>());
                M30 = static_cast<float>(Ar.Read<double>()); M31 = static_cast<float>(Ar.Read<double>());
                M32 = static_cast<float>(Ar.Read<double>()); M33 = static_cast<float>(Ar.Read<double>());
            }
            else
            {
                M00 = Ar.Read<float>(); M01 = Ar.Read<float>(); M02 = Ar.Read<float>(); M03 = Ar.Read<float>();
                M10 = Ar.Read<float>(); M11 = Ar.Read<float>(); M12 = Ar.Read<float>(); M13 = Ar.Read<float>();
                M20 = Ar.Read<float>(); M21 = Ar.Read<float>(); M22 = Ar.Read<float>(); M23 = Ar.Read<float>();
                M30 = Ar.Read<float>(); M31 = Ar.Read<float>(); M32 = Ar.Read<float>(); M33 = Ar.Read<float>();
            }
        }

        friend FMatrix operator*(const FMatrix& a, const FMatrix& b)
        {
            return FMatrix(
                a.M00 * b.M00 + a.M01 * b.M10 + a.M02 * b.M20 + a.M03 * b.M30,
                a.M00 * b.M01 + a.M01 * b.M11 + a.M02 * b.M21 + a.M03 * b.M31,
                a.M00 * b.M02 + a.M01 * b.M12 + a.M02 * b.M22 + a.M03 * b.M32,
                a.M00 * b.M03 + a.M01 * b.M13 + a.M02 * b.M23 + a.M03 * b.M33,
                a.M10 * b.M00 + a.M11 * b.M10 + a.M12 * b.M20 + a.M13 * b.M30,
                a.M10 * b.M01 + a.M11 * b.M11 + a.M12 * b.M21 + a.M13 * b.M31,
                a.M10 * b.M02 + a.M11 * b.M12 + a.M12 * b.M22 + a.M13 * b.M32,
                a.M10 * b.M03 + a.M11 * b.M13 + a.M12 * b.M23 + a.M13 * b.M33,
                a.M20 * b.M00 + a.M21 * b.M10 + a.M22 * b.M20 + a.M23 * b.M30,
                a.M20 * b.M01 + a.M21 * b.M11 + a.M22 * b.M21 + a.M23 * b.M31,
                a.M20 * b.M02 + a.M21 * b.M12 + a.M22 * b.M22 + a.M23 * b.M32,
                a.M20 * b.M03 + a.M21 * b.M13 + a.M22 * b.M23 + a.M23 * b.M33,
                a.M30 * b.M00 + a.M31 * b.M10 + a.M32 * b.M20 + a.M33 * b.M30,
                a.M30 * b.M01 + a.M31 * b.M11 + a.M32 * b.M21 + a.M33 * b.M31,
                a.M30 * b.M02 + a.M31 * b.M12 + a.M32 * b.M22 + a.M33 * b.M32,
                a.M30 * b.M03 + a.M31 * b.M13 + a.M32 * b.M23 + a.M33 * b.M33);
        }

        float operator[](int i) const
        {
            switch (i)
            {
                case 0: return M00; case 1: return M01; case 2: return M02; case 3: return M03;
                case 4: return M10; case 5: return M11; case 6: return M12; case 7: return M13;
                case 8: return M20; case 9: return M21; case 10: return M22; case 11: return M23;
                case 12: return M30; case 13: return M31; case 14: return M32; case 15: return M33;
                default: throw std::out_of_range("FMatrix index");
            }
        }
        float& operator[](int i)
        {
            switch (i)
            {
                case 0: return M00; case 1: return M01; case 2: return M02; case 3: return M03;
                case 4: return M10; case 5: return M11; case 6: return M12; case 7: return M13;
                case 8: return M20; case 9: return M21; case 10: return M22; case 11: return M23;
                case 12: return M30; case 13: return M31; case 14: return M32; case 15: return M33;
                default: throw std::out_of_range("FMatrix index");
            }
        }

        FVector4 TransformFVector4(const FVector4& p) const
        {
            return FVector4(
                p.X * M00 + p.Y * M10 + p.Z * M20 + p.W * M30,
                p.X * M01 + p.Y * M11 + p.Z * M21 + p.W * M31,
                p.X * M02 + p.Y * M12 + p.Z * M22 + p.W * M32,
                p.X * M03 + p.Y * M13 + p.Z * M23 + p.W * M33);
        }

        FVector4 TransformPosition(const FVector& v) const { return TransformFVector4(FVector4(v.X, v.Y, v.Z, 1.0f)); }

        FVector InverseTransformPosition(const FVector& v) const
        {
            FMatrix invSelf = InverseFast();
            return static_cast<FVector>(invSelf.TransformPosition(v));
        }

        FVector4 TransformVector(const FVector& v) const { return TransformFVector4(FVector4(v.X, v.Y, v.Z, 0.0f)); }

        FMatrix GetTransposed() const
        {
            return FMatrix(
                M00, M10, M20, M30,
                M01, M11, M21, M31,
                M02, M12, M22, M32,
                M03, M13, M23, M33);
        }

        float Determinant() const
        {
            return M00 * (
                    M11 * (M22 * M33 - M23 * M32) -
                    M21 * (M12 * M33 - M13 * M32) +
                    M31 * (M12 * M23 - M13 * M22)) -
                M10 * (
                    M01 * (M22 * M33 - M23 * M32) -
                    M21 * (M02 * M33 - M03 * M32) +
                    M31 * (M02 * M23 - M03 * M22)) +
                M20 * (
                    M01 * (M12 * M33 - M13 * M32) -
                    M11 * (M02 * M33 - M03 * M32) +
                    M31 * (M02 * M13 - M03 * M12)) -
                M30 * (
                    M01 * (M12 * M23 - M13 * M22) -
                    M11 * (M02 * M23 - M03 * M22) +
                    M21 * (M02 * M13 - M03 * M12));
        }

        float RotDeterminant() const
        {
            return M00 * (M11 * M22 - M12 * M21) -
                M10 * (M01 * M22 - M02 * M21) +
                M20 * (M01 * M12 - M02 * M11);
        }

        FMatrix InverseFast() const
        {
            FMatrix result;
            float det[4];
            FMatrix tmp;

            tmp.M00 = M22 * M33 - M23 * M32;
            tmp.M01 = M12 * M33 - M13 * M32;
            tmp.M02 = M12 * M23 - M13 * M22;

            tmp.M10 = M22 * M33 - M23 * M32;
            tmp.M11 = M02 * M33 - M03 * M32;
            tmp.M12 = M02 * M23 - M03 * M22;

            tmp.M20 = M12 * M33 - M13 * M32;
            tmp.M21 = M02 * M33 - M03 * M32;
            tmp.M22 = M02 * M13 - M03 * M12;

            tmp.M30 = M12 * M23 - M13 * M22;
            tmp.M31 = M02 * M23 - M03 * M22;
            tmp.M32 = M02 * M13 - M03 * M12;

            det[0] = M11 * tmp.M00 - M21 * tmp.M01 + M31 * tmp.M02;
            det[1] = M01 * tmp.M10 - M21 * tmp.M11 + M31 * tmp.M12;
            det[2] = M01 * tmp.M20 - M11 * tmp.M21 + M31 * tmp.M22;
            det[3] = M01 * tmp.M30 - M11 * tmp.M31 + M21 * tmp.M32;

            const float determinant = M00 * det[0] - M10 * det[1] + M20 * det[2] - M30 * det[3];
            const float rDet = 1.0f / determinant;

            result.M00 = rDet * det[0];
            result.M01 = -rDet * det[1];
            result.M02 = rDet * det[2];
            result.M03 = -rDet * det[3];
            result.M10 = -rDet * (M10 * tmp.M00 - M20 * tmp.M01 + M30 * tmp.M02);
            result.M11 = rDet * (M00 * tmp.M10 - M20 * tmp.M11 + M30 * tmp.M12);
            result.M12 = -rDet * (M00 * tmp.M20 - M10 * tmp.M21 + M30 * tmp.M22);
            result.M13 = rDet * (M00 * tmp.M30 - M10 * tmp.M31 + M20 * tmp.M32);
            result.M20 = rDet * (
                M10 * (M21 * M33 - M23 * M31) -
                M20 * (M11 * M33 - M13 * M31) +
                M30 * (M11 * M23 - M13 * M21));
            result.M21 = -rDet * (
                M00 * (M21 * M33 - M23 * M31) -
                M20 * (M01 * M33 - M03 * M31) +
                M30 * (M01 * M23 - M03 * M21));
            result.M22 = rDet * (
                M00 * (M11 * M33 - M13 * M31) -
                M10 * (M01 * M33 - M03 * M31) +
                M30 * (M01 * M13 - M03 * M11));
            result.M23 = -rDet * (
                M00 * (M11 * M23 - M13 * M21) -
                M10 * (M01 * M23 - M03 * M21) +
                M20 * (M01 * M13 - M03 * M11));
            result.M30 = -rDet * (
                M10 * (M21 * M32 - M22 * M31) -
                M20 * (M11 * M32 - M12 * M31) +
                M30 * (M11 * M22 - M12 * M21));
            result.M31 = rDet * (
                M00 * (M21 * M32 - M22 * M31) -
                M20 * (M01 * M32 - M02 * M31) +
                M30 * (M01 * M22 - M02 * M21));
            result.M32 = -rDet * (
                M00 * (M11 * M32 - M12 * M31) -
                M10 * (M01 * M32 - M02 * M31) +
                M30 * (M01 * M12 - M02 * M11));
            result.M33 = rDet * (
                M00 * (M11 * M22 - M12 * M21) -
                M10 * (M01 * M22 - M02 * M21) +
                M20 * (M01 * M12 - M02 * M11));

            return result;
        }

        FMatrix Inverse() const
        {
            // Check for zero scale matrix to invert.
            if (GetScaledAxis(EAxis::X).IsNearlyZero(UnrealMath::SmallNumber) &&
                GetScaledAxis(EAxis::Y).IsNearlyZero(UnrealMath::SmallNumber) &&
                GetScaledAxis(EAxis::Z).IsNearlyZero(UnrealMath::SmallNumber))
            {
                // just set to zero - avoids unsafe inverse of zero.
                return Identity();
            }

            const float det = Determinant();
            return det == 0.0f ? Identity() : InverseFast();
        }

        void RemoveScaling(float tolerance = UnrealMath::SmallNumber)
        {
            const float squareSum0 = M00 * M00 + M01 * M01 + M02 * M02;
            const float squareSum1 = M10 * M10 + M11 * M11 + M12 * M12;
            const float squareSum2 = M20 * M20 + M21 * M21 + M22 * M22;

            const float scale0 = squareSum0 - tolerance >= 0 ? CUE4Parse::Utils::InvSqrt(squareSum0) : 1;
            const float scale1 = squareSum1 - tolerance >= 0 ? CUE4Parse::Utils::InvSqrt(squareSum1) : 1;
            const float scale2 = squareSum2 - tolerance >= 0 ? CUE4Parse::Utils::InvSqrt(squareSum2) : 1;

            M00 *= scale0; M01 *= scale0; M02 *= scale0;
            M10 *= scale1; M11 *= scale1; M12 *= scale1;
            M20 *= scale2; M21 *= scale2; M22 *= scale2;
        }

        FVector ExtractScaling(float tolerance = UnrealMath::SmallNumber)
        {
            const float squareSum0 = M00 * M00 + M01 * M01 + M02 * M02;
            const float squareSum1 = M10 * M10 + M11 * M11 + M12 * M12;
            const float squareSum2 = M20 * M20 + M21 * M21 + M22 * M22;

            FVector scale3D;

            if (squareSum0 > tolerance)
            {
                const float scale0 = std::sqrt(squareSum0);
                scale3D.X = scale0;
                const float invScale0 = 1.0f / scale0;
                M00 *= invScale0; M01 *= invScale0; M02 *= invScale0;
            }
            else { scale3D.X = 0.0f; }

            if (squareSum1 > tolerance)
            {
                const float scale1 = std::sqrt(squareSum1);
                scale3D.Y = scale1;
                const float invScale1 = 1.0f / scale1;
                M10 *= invScale1; M11 *= invScale1; M12 *= invScale1;
            }
            else { scale3D.Y = 0.0f; }

            if (squareSum2 > tolerance)
            {
                const float scale2 = std::sqrt(squareSum2);
                scale3D.Z = scale2;
                const float invScale2 = 1.0f / scale2;
                M20 *= invScale2; M21 *= invScale2; M22 *= invScale2;
            }
            else { scale3D.Z = 0.0f; }

            return scale3D;
        }

        float GetMaximumAxisScale() const
        {
            const float maxRowScaleSquared = std::max(
                GetScaledAxis(EAxis::X).SizeSquared(),
                std::max(
                    GetScaledAxis(EAxis::Y).SizeSquared(),
                    GetScaledAxis(EAxis::Z).SizeSquared()));
            return std::sqrt(maxRowScaleSquared);
        }

        FVector GetOrigin() const { return FVector(M30, M31, M32); }

        FVector GetScaledAxis(EAxis axis) const
        {
            switch (axis)
            {
                case EAxis::X: return FVector(M00, M01, M02);
                case EAxis::Y: return FVector(M10, M11, M12);
                case EAxis::Z: return FVector(M20, M21, M22);
                default: return FVector::ZeroVector;
            }
        }

        void SetAxis(int i, const FVector& axis)
        {
            switch (i)
            {
                case 0: M00 = axis.X; M01 = axis.Y; M02 = axis.Z; break;
                case 1: M10 = axis.X; M11 = axis.Y; M12 = axis.Z; break;
                case 2: M20 = axis.X; M21 = axis.Y; M22 = axis.Z; break;
                case 3: M30 = axis.X; M31 = axis.Y; M32 = axis.Z; break;
                default: break;
            }
        }

        FVector GetScaleVector(float tolerance = UnrealMath::SmallNumber) const
        {
            FVector scale3D(1, 1, 1);

            const float squareSum0 = M00 * M00 + M01 * M01 + M02 * M02;
            scale3D[0] = squareSum0 > tolerance ? std::sqrt(squareSum0) : 0.0f;

            const float squareSum1 = M10 * M10 + M11 * M11 + M12 * M12;
            scale3D[1] = squareSum1 > tolerance ? std::sqrt(squareSum1) : 0.0f;

            const float squareSum2 = M20 * M20 + M21 * M21 + M22 * M22;
            scale3D[2] = squareSum2 > tolerance ? std::sqrt(squareSum2) : 0.0f;

            return scale3D;
        }

        // Cross-type members — defined in Matrix.cpp where FRotator / FQuat / FPlane are complete.
        FRotator Rotator() const;
        FQuat ToQuat() const;
        bool MakeFrustumPlane(float a, float b, float c, float d, FPlane& plane) const;
        bool GetFrustumNearPlane(FPlane& plane) const;
        bool GetFrustumFarPlane(FPlane& plane) const;
        bool GetFrustumLeftPlane(FPlane& plane) const;
        bool GetFrustumRightPlane(FPlane& plane) const;
        bool GetFrustumTopPlane(FPlane& plane) const;
        bool GetFrustumBottomPlane(FPlane& plane) const;

        std::string ToString() const
        {
            char buf[192];
            std::snprintf(buf, sizeof(buf),
                "[%.1f %.1f %.1f %.1f] [%.1f %.1f %.1f %.1f] [%.1f %.1f %.1f %.1f] [%.1f %.1f %.1f %.1f]",
                M00, M01, M02, M03, M10, M11, M12, M13, M20, M21, M22, M23, M30, M31, M32, M33);
            return buf;
        }
    };
}
