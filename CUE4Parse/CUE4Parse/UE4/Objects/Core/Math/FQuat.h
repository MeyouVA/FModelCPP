// Ported from CUE4Parse/UE4/Objects/Core/Math/FQuat.cs — the rotation quaternion.
// USE Ar.Read<FQuat> FOR FLOATS AND FQuat(Ar) FOR DOUBLES (via ReadFReal).
//
// Deferred / deviations:
//   - the TIntVector4<float>/<double> ctors (TIntVector4 not ported) and the System.Numerics /
//     FixedMathSharp conversions (no such types here).
//   - operator*(FQuat,FQuat) uses only the portable scalar path; C#'s SSE VectorQuaternionMultiply2
//     fast path is omitted (the scalar branch is what UnrealMathSSE mirrors and is fully portable).
//   - Serialize(FArchiveWriter) / GetHashCode / Equals(object) — not on the parse path.
//
// The ctors from FMatrix / FRotator and Rotator() (which touch those types) are declared here and
// defined in FQuat.cpp.
#pragma once

#include <cmath>
#include <cstdio>
#include <string>

#include "FVector.h"
#include "EForceInit.h"
#include "UnrealMathUtility.h"
#include "../../../Readers/FArchive.h"
#include "../../../../Utils/MathUtils.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    using CUE4Parse::UE4::Readers::FArchive;

    class FMatrix;   // Matrix.h
    struct FRotator; // FRotator.h

    struct FQuat
    {
        static constexpr float THRESH_QUAT_NORMALIZED = 0.01f; // Allowed error for a normalized quaternion.

        static const FQuat Identity;

        float X = 0.0f;
        float Y = 0.0f;
        float Z = 0.0f;
        float W = 0.0f;

        FQuat() = default;

        explicit FQuat(EForceInit zeroOrNot)
        {
            X = 0; Y = 0; Z = 0;
            W = zeroOrNot == EForceInit::ForceInitToZero ? 0.0f : 1.0f;
        }

        FQuat(float x, float y, float z, float w) : X(x), Y(y), Z(z), W(w) {}

        explicit FQuat(FArchive& Ar) { X = Ar.ReadFReal(); Y = Ar.ReadFReal(); Z = Ar.ReadFReal(); W = Ar.ReadFReal(); }

        // Cross-type ctors — defined in FQuat.cpp.
        explicit FQuat(const FMatrix& m);
        explicit FQuat(const FRotator& rotator);

        FQuat(const FVector& axis, float angleRad)
        {
            const float halfA = 0.5f * angleRad;
            const float s = std::sin(halfA);
            const float c = std::cos(halfA);
            X = s * axis.X;
            Y = s * axis.Y;
            Z = s * axis.Z;
            W = c;
        }

        bool Equals(const FQuat& q, float tolerance) const
        {
            return (std::fabs(X - q.X) <= tolerance && std::fabs(Y - q.Y) <= tolerance && std::fabs(Z - q.Z) <= tolerance && std::fabs(W - q.W) <= tolerance) ||
                (std::fabs(X + q.X) <= tolerance && std::fabs(Y + q.Y) <= tolerance && std::fabs(Z + q.Z) <= tolerance && std::fabs(W + q.W) <= tolerance);
        }
        bool Equals(const FQuat& other) const { return Equals(other, UnrealMath::KindaSmallNumber); }

        bool IsIdentity(float tolerance = UnrealMath::SmallNumber) const { return Equals(Identity, tolerance); }
        bool IsVectorZero() const { return X == 0 && Y == 0 && Z == 0; }

        // C#'s operator * (quaternion composition). Scalar path only (see header note).
        friend FQuat operator*(const FQuat& a, const FQuat& b)
        {
            FQuat r;
            const float t0 = (a.Z - a.Y) * (b.Y - b.Z);
            const float t1 = (a.W + a.X) * (b.W + b.X);
            const float t2 = (a.W - a.X) * (b.Y + b.Z);
            const float t3 = (a.Y + a.Z) * (b.W - b.X);
            const float t4 = (a.Z - a.X) * (b.X - b.Y);
            const float t5 = (a.Z + a.X) * (b.X + b.Y);
            const float t6 = (a.W + a.Y) * (b.W - b.Z);
            const float t7 = (a.W - a.Y) * (b.W + b.Z);
            const float t8 = t5 + t6 + t7;
            const float t9 = 0.5f * (t4 + t8);

            r.X = t1 + t9 - t8;
            r.Y = t2 + t9 - t7;
            r.Z = t3 + t9 - t6;
            r.W = t0 + t9 - t5;
            return r;
        }

        friend FVector operator*(const FQuat& a, const FVector& b) { return a.RotateVector(b); }

        void Normalize(float tolerance = UnrealMath::SmallNumber)
        {
            const float squareSum = X * X + Y * Y + Z * Z + W * W;
            if (squareSum >= tolerance)
            {
                const float scale = CUE4Parse::Utils::InvSqrt(squareSum);
                X *= scale; Y *= scale; Z *= scale; W *= scale;
            }
            else
            {
                *this = Identity;
            }
        }

        FQuat GetNormalized(float tolerance = UnrealMath::SmallNumber) const
        {
            FQuat result = *this;
            result.Normalize(tolerance);
            return result;
        }

        bool IsNormalized() const { return std::fabs(1.0f - SizeSquared()) < THRESH_QUAT_NORMALIZED; }
        float Size() const { return std::sqrt(SizeSquared()); }
        float SizeSquared() const { return X * X + Y * Y + Z * Z + W * W; }

        FVector RotateVector(const FVector& v) const
        {
            // V' = V + 2w(Q x V) + (2Q x (Q x V)); refactored as V + w*T + (Q x T), T = 2(Q x V).
            const FVector q(X, Y, Z);
            const FVector t = 2.0f * FVector::CrossProduct(q, v);
            return v + (W * t) + FVector::CrossProduct(q, t);
        }

        FVector UnrotateVector(const FVector& v) const
        {
            const FVector q(-X, -Y, -Z); // Inverse
            const FVector t = 2.0f * FVector::CrossProduct(q, v);
            return v + (W * t) + FVector::CrossProduct(q, t);
        }

        FQuat Inverse() const { return IsNormalized() ? FQuat(-X, -Y, -Z, W) : GetNormalized().Inverse(); }

        void Conjugate() { X = -X; Y = -Y; Z = -Z; }
        static FQuat Conjugate(const FQuat& quat) { return FQuat(-quat.X, -quat.Y, -quat.Z, quat.W); }

        static FQuat FindBetweenNormals(const FVector& a, const FVector& b, float normAb = 1.0f)
        {
            float w = normAb + FVector::DotProduct(a, b);
            FQuat result;

            if (w >= 1e-6f * normAb)
            {
                result = FQuat(
                    a.Y * b.Z - a.Z * b.Y,
                    a.Z * b.X - a.X * b.Z,
                    a.X * b.Y - a.Y * b.X,
                    w);
            }
            else
            {
                w = 0.0f;
                result = std::fabs(a.X) > std::fabs(a.Y)
                    ? FQuat(-a.Z, 0.0f, a.X, w)
                    : FQuat(0.0f, -a.Z, a.Y, w);
            }

            result.Normalize();
            return result;
        }

        // Defined in FQuat.cpp (returns FRotator).
        FRotator Rotator() const;

        bool ContainsNaN() const
        {
            return !std::isfinite(X) || !std::isfinite(Y) || !std::isfinite(Z) || !std::isfinite(W);
        }

        std::string ToString() const
        {
            char buf[96];
            std::snprintf(buf, sizeof(buf), "X=%.3f Y=%.3f Z=%.3f W=%.3f", X, Y, Z, W);
            return buf;
        }

        static FQuat FastLerp(const FQuat& q1, const FQuat& q2, float alpha)
        {
            const float doResult = q1 | q2;
            const float bias = CUE4Parse::Utils::FloatSelect(doResult, 1.0f, -1.0f);
            return (q2 * alpha) + (q1 * (bias * (1.0f - alpha)));
        }

        static FQuat Slerp_NotNormalized(const FQuat& quat1, const FQuat& quat2, float slerp)
        {
            const float rawCosom =
                quat1.X * quat2.X +
                quat1.Y * quat2.Y +
                quat1.Z * quat2.Z +
                quat1.W * quat2.W;
            const float cosom = CUE4Parse::Utils::FloatSelect(rawCosom, rawCosom, -rawCosom);

            float scale0, scale1;
            if (cosom < 0.9999f)
            {
                const float omega = std::acos(cosom);
                const float invSin = 1.0f / std::sin(omega);
                scale0 = std::sin((1.0f - slerp) * omega) * invSin;
                scale1 = std::sin(slerp * omega) * invSin;
            }
            else
            {
                scale0 = 1.0f - slerp;
                scale1 = slerp;
            }

            scale1 = CUE4Parse::Utils::FloatSelect(rawCosom, scale1, -scale1);

            FQuat r;
            r.X = scale0 * quat1.X + scale1 * quat2.X;
            r.Y = scale0 * quat1.Y + scale1 * quat2.Y;
            r.Z = scale0 * quat1.Z + scale1 * quat2.Z;
            r.W = scale0 * quat1.W + scale1 * quat2.W;
            return r;
        }

        static FQuat Slerp(const FQuat& quat1, const FQuat& quat2, float slerp) { return Slerp_NotNormalized(quat1, quat2, slerp).GetNormalized(); }

        friend float operator|(const FQuat& a, const FQuat& b) { return a.X * b.X + a.Y * b.Y + a.Z * b.Z + a.W * b.W; }
        friend FQuat operator*(const FQuat& a, float scale) { return FQuat(scale * a.X, scale * a.Y, scale * a.Z, scale * a.W); }
        friend FQuat operator+(const FQuat& a, const FQuat& b) { return FQuat(a.X + b.X, a.Y + b.Y, a.Z + b.Z, a.W + b.W); }

        friend bool operator==(const FQuat& a, const FQuat& b) { return a.Equals(b); }
        friend bool operator!=(const FQuat& a, const FQuat& b) { return !a.Equals(b); }
    };

    inline const FQuat FQuat::Identity{0, 0, 0, 1};
}
