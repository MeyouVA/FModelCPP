// Ported from CUE4Parse/UE4/Objects/Core/Math/FVector.cs — the 3D float vector, the keystone math type.
// USE Ar.Read<FVector> FOR FLOATS AND FVector(Ar) FOR DOUBLES (via ReadFReal).
//
// Deliberately deferred (need layers not yet ported):
//   - ToOrientationRotator / ToOrientationQuat / Rotation and operator*(FVector, FQuat)  → wait on FRotator/FQuat
//   - Serialize(FArchiveWriter)                                                          → no Writers layer
//   - the System.Numerics Vector3 and FixedMathSharp Vector3d conversions               → no such types here
//   - GetHashCode / Equals(object)                                                       → not on the parse path
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>

#include "FLinearColor.h"
#include "FVector2D.h"
#include "FVector4.h"
#include "FIntVector.h"
#include "FIntPoint.h"
#include "UnrealMathUtility.h"
#include "../../../Readers/FArchive.h"
#include "../../../../Utils/MathUtils.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    using CUE4Parse::UE4::Readers::FArchive;

    struct FVector
    {
        // Allowed error for a normalized vector (against squared magnitude).
        static constexpr float ThreshVectorNormalized = 0.01f;

        static const FVector ZeroVector;
        static const FVector OneVector;
        static const FVector UpVector;
        static const FVector ForwardVector;
        static const FVector RightVector;

        float X = 0.0f;
        float Y = 0.0f;
        float Z = 0.0f;

        FVector() = default;
        FVector(float x, float y, float z) : X(x), Y(y), Z(z) {}
        // C#'s FVector(double,double,double) truncating ctor is intentionally omitted: keeping it alongside the
        // float ctor makes every all-int construction (e.g. FVector(1,2,3)) ambiguous in C++ (C# instead prefers
        // the float overload). The few double sources cast to float explicitly at the call site.
        explicit FVector(FArchive& Ar) { X = Ar.ReadFReal(); Y = Ar.ReadFReal(); Z = Ar.ReadFReal(); }
        explicit FVector(float f) : FVector(f, f, f) {}
        FVector(FVector2D v, float z) : X(v.X), Y(v.Y), Z(z) {}
        explicit FVector(const FVector4& v); // defined below (FVector4 is complete here)
        FVector(const FLinearColor& color) : X(color.R), Y(color.G), Z(color.B) {}
        FVector(const FIntVector& v)
            : X(static_cast<float>(v.X)), Y(static_cast<float>(v.Y)), Z(static_cast<float>(v.Z)) {}
        FVector(const FIntPoint& p) : X(static_cast<float>(p.X)), Y(static_cast<float>(p.Y)), Z(0.0f) {}

        FVector Set(FVector other) { X = other.X; Y = other.Y; Z = other.Z; return *this; }
        FVector Set(float x, float y, float z) { X = x; Y = y; Z = z; return *this; }

        FVector GetSignVector() const
        {
            FVector v;
            v.X = X >= 0 ? 1.0f : -1.0f;
            v.Y = Y >= 0 ? 1.0f : -1.0f;
            v.Z = Z >= 0 ? 1.0f : -1.0f;
            return v;
        }

        void Scale(float scale) { X *= scale; Y *= scale; Z *= scale; }
        void Scale(FVector scale) { X *= scale.X; Y *= scale.Y; Z *= scale.Z; }

        // Unary + / - .
        friend FVector operator+(FVector a) { return a; }
        friend FVector operator-(FVector a) { return {-a.X, -a.Y, -a.Z}; }

        // Cross product (C#'s operator ^).
        friend FVector operator^(FVector a, FVector b)
        {
            return {a.Y * b.Z - a.Z * b.Y, a.Z * b.X - a.X * b.Z, a.X * b.Y - a.Y * b.X};
        }
        // Dot product (C#'s operator |).
        friend float operator|(FVector a, FVector b) { return a.X * b.X + a.Y * b.Y + a.Z * b.Z; }

        friend FVector operator+(FVector a, FVector b) { return {a.X + b.X, a.Y + b.Y, a.Z + b.Z}; }
        friend FVector operator+(FVector a, const FIntVector& b) { return {a.X + b.X, a.Y + b.Y, a.Z + b.Z}; }
        friend FVector operator+(FVector a, float bias) { return {a.X + bias, a.Y + bias, a.Z + bias}; }
        friend FVector operator-(FVector a, FVector b) { return {a.X - b.X, a.Y - b.Y, a.Z - b.Z}; }
        friend FVector operator-(FVector a, const FIntVector& b) { return {a.X - b.X, a.Y - b.Y, a.Z - b.Z}; }
        friend FVector operator-(FVector a, float bias) { return {a.X - bias, a.Y - bias, a.Z - bias}; }
        friend FVector operator*(FVector a, FVector b) { return {a.X * b.X, a.Y * b.Y, a.Z * b.Z}; }
        friend FVector operator*(FVector a, const FIntVector& b) { return {a.X * b.X, a.Y * b.Y, a.Z * b.Z}; }
        friend FVector operator*(FVector a, float scale) { return {a.X * scale, a.Y * scale, a.Z * scale}; }
        friend FVector operator*(float scale, FVector a) { return a * scale; }
        friend FVector operator/(FVector a, FVector b) { return {a.X / b.X, a.Y / b.Y, a.Z / b.Z}; }
        friend FVector operator/(FVector a, const FIntVector& b) { return {a.X / b.X, a.Y / b.Y, a.Z / b.Z}; }
        friend FVector operator/(FVector a, float scale)
        {
            const float rScale = 1.0f / scale;
            return {a.X * rScale, a.Y * rScale, a.Z * rScale};
        }
        friend FVector operator/(float scale, FVector a) { return a / scale; }

        friend bool operator==(FVector a, FVector b) { return a.X == b.X && a.Y == b.Y && a.Z == b.Z; }
        friend bool operator!=(FVector a, FVector b) { return a.X != b.X || a.Y != b.Y || a.Z != b.Z; }

        float operator[](int i) const
        {
            switch (i) { case 0: return X; case 1: return Y; case 2: return Z; default: throw std::out_of_range("FVector index"); }
        }
        float& operator[](int i)
        {
            switch (i) { case 0: return X; case 1: return Y; case 2: return Z; default: throw std::out_of_range("FVector index"); }
        }

        bool Equals(FVector v, float tolerance) const
        {
            return std::fabs(X - v.X) <= tolerance && std::fabs(Y - v.Y) <= tolerance && std::fabs(Z - v.Z) <= tolerance;
        }
        bool Equals(FVector v) const { return Equals(v, UnrealMath::KindaSmallNumber); }

        bool AllComponentsEqual(float tolerance = UnrealMath::KindaSmallNumber) const
        {
            return std::fabs(X - Y) <= tolerance && std::fabs(X - Z) <= tolerance && std::fabs(Y - Z) <= tolerance;
        }

        float Max() const { return std::max(std::max(X, Y), Z); }
        float AbsMax() const { return std::max(std::max(std::fabs(X), std::fabs(Y)), std::fabs(Z)); }
        float Min() const { return std::min(std::min(X, Y), Z); }
        float AbsMin() const { return std::min(std::min(std::fabs(X), std::fabs(Y)), std::fabs(Z)); }

        FVector ComponentMax(FVector other) const { return {std::max(X, other.X), std::max(Y, other.Y), std::max(Z, other.Z)}; }
        FVector ComponentMin(FVector other) const { return {std::min(X, other.X), std::min(Y, other.Y), std::min(Z, other.Z)}; }
        FVector Abs() const { return {std::fabs(X), std::fabs(Y), std::fabs(Z)}; }

        float Size() const { return std::sqrt(X * X + Y * Y + Z * Z); }
        float SizeSquared() const { return X * X + Y * Y + Z * Z; }
        float Size2D() const { return std::sqrt(X * X + Y * Y); }
        float SizeSquared2D() const { return X * X + Y * Y; }

        bool ContainsNaN() const { return !std::isfinite(X) || !std::isfinite(Y) || !std::isfinite(Z); }

        bool IsNearlyZero(float tolerance = UnrealMath::KindaSmallNumber) const
        {
            return std::fabs(X) <= tolerance && std::fabs(Y) <= tolerance && std::fabs(Z) <= tolerance;
        }
        bool IsZero() const { return X == 0 && Y == 0 && Z == 0; }
        bool IsUnit(float lengthSquaredTolerance = UnrealMath::KindaSmallNumber) const
        {
            return std::fabs(1.0f - SizeSquared()) < lengthSquaredTolerance;
        }
        bool IsNormalized() const { return std::fabs(1.0f - SizeSquared()) < ThreshVectorNormalized; }

        bool Normalize(float tolerance = UnrealMath::SmallNumber)
        {
            const float squareSum = X * X + Y * Y + Z * Z;
            if (squareSum > tolerance)
            {
                const float scale = CUE4Parse::Utils::InvSqrt(squareSum);
                X *= scale; Y *= scale; Z *= scale;
                return true;
            }
            return false;
        }

        FVector GetClampedToMaxSize(float maxSize) const
        {
            if (maxSize < UnrealMath::KindaSmallNumber)
                return FVector(0.0f, 0.0f, 0.0f);

            const float vSq = SizeSquared();
            if (vSq > maxSize * maxSize)
            {
                const float scale = maxSize * CUE4Parse::Utils::InvSqrt(vSq);
                return {X * scale, Y * scale, Z * scale};
            }
            return {X, Y, Z};
        }

        FVector GetSafeNormal(float tolerance = UnrealMath::SmallNumber) const
        {
            const float squareSum = X * X + Y * Y + Z * Z;
            if (squareSum == 1.0f) return *this;
            if (squareSum < tolerance) return ZeroVector;
            const float scale = CUE4Parse::Utils::InvSqrt(squareSum);
            return {X * scale, Y * scale, Z * scale};
        }

        FVector GetSafeNormal2D(float tolerance = UnrealMath::SmallNumber) const
        {
            const float squareSum = X * X + Y * Y;
            if (squareSum == 1.0f)
                return Z == 0.0f ? *this : FVector(X, Y, 0.0f);
            if (squareSum < tolerance) return ZeroVector;
            const float scale = CUE4Parse::Utils::InvSqrt(squareSum);
            return {X * scale, Y * scale, 0.0f};
        }

        float CosineAngle2D(FVector b) const
        {
            FVector a = *this;
            a.Z = 0.0f;
            b.Z = 0.0f;
            a.Normalize();
            b.Normalize();
            return a | b;
        }

        FVector ProjectOnTo(FVector a) const { return a * ((*this | a) / (a | a)); }
        FVector ProjectOnToNormal(FVector normal) const { return normal * (*this | normal); }

        std::string ToString() const
        {
            char buf[96];
            std::snprintf(buf, sizeof(buf), "X=%.3f Y=%.3f Z=%.3f", X, Y, Z);
            return buf;
        }

        static FVector CrossProduct(FVector a, FVector b) { return a ^ b; }
        static float DotProduct(FVector a, FVector b) { return a | b; }

        static float ComputeSquaredDistanceFromBoxToPoint(FVector mins, FVector maxs, FVector point)
        {
            float distSquared = 0.0f;
            if (point.X < mins.X) distSquared += (point.X - mins.X) * (point.X - mins.X);
            else if (point.X > maxs.X) distSquared += (point.X - maxs.X) * (point.X - maxs.X);
            if (point.Y < mins.Y) distSquared += (point.Y - mins.Y) * (point.Y - mins.Y);
            else if (point.Y > maxs.Y) distSquared += (point.Y - maxs.Y) * (point.Y - maxs.Y);
            if (point.Z < mins.Z) distSquared += (point.Z - mins.Z) * (point.Z - mins.Z);
            else if (point.Z > maxs.Z) distSquared += (point.Z - maxs.Z) * (point.Z - maxs.Z);
            return distSquared;
        }

        static float PointPlaneDist(FVector point, FVector planeBase, FVector planeNormal)
        {
            return (point - planeBase) | planeNormal;
        }
        static FVector PointPlaneProject(FVector point, FVector planeBase, FVector planeNormal)
        {
            return point - PointPlaneDist(point, planeBase, planeNormal) * planeNormal;
        }
        static FVector VectorPlaneProject(FVector delta, FVector normal) { return delta - delta.ProjectOnToNormal(normal); }

        static float DistSquared(FVector v1, FVector v2)
        {
            return CUE4Parse::Utils::Square(v2.X - v1.X) + CUE4Parse::Utils::Square(v2.Y - v1.Y) + CUE4Parse::Utils::Square(v2.Z - v1.Z);
        }
    };

    inline const FVector FVector::ZeroVector{0.0f, 0.0f, 0.0f};
    inline const FVector FVector::OneVector{1.0f, 1.0f, 1.0f};
    inline const FVector FVector::UpVector{0.0f, 0.0f, 1.0f};
    inline const FVector FVector::ForwardVector{1.0f, 0.0f, 0.0f};
    inline const FVector FVector::RightVector{0.0f, 1.0f, 0.0f};

    // FVector's own ctor from FVector4 (FVector4 is complete here).
    inline FVector::FVector(const FVector4& v) : X(v.X), Y(v.Y), Z(v.Z) {}

    // The double-precision sibling (FVector3d in C#).
    struct FVector3d
    {
        double X = 0.0;
        double Y = 0.0;
        double Z = 0.0;

        static const FVector3d ZeroVector;

        FVector3d() = default;
        FVector3d(float x, float y, float z) : X(x), Y(y), Z(z) {}
        FVector3d(double x, double y, double z) : X(x), Y(y), Z(z) {}
        explicit FVector3d(FArchive& Ar) { X = Ar.Read<double>(); Y = Ar.Read<double>(); Z = Ar.Read<double>(); }

        friend FVector3d operator+(FVector3d a, FVector3d b) { return {a.X + b.X, a.Y + b.Y, a.Z + b.Z}; }
        friend FVector3d operator-(FVector3d a, FVector3d b) { return {a.X - b.X, a.Y - b.Y, a.Z - b.Z}; }
        friend FVector3d operator*(FVector3d a, FVector3d b) { return {a.X * b.X, a.Y * b.Y, a.Z * b.Z}; }
        friend FVector3d operator/(FVector3d a, FVector3d b) { return {a.X / b.X, a.Y / b.Y, a.Z / b.Z}; }
        friend FVector3d operator+(FVector3d a, double b) { return {a.X + b, a.Y + b, a.Z + b}; }
        friend FVector3d operator*(FVector3d a, double b) { return {a.X * b, a.Y * b, a.Z * b}; }
        friend FVector3d operator-(FVector3d a, double b) { return {a.X - b, a.Y - b, a.Z - b}; }

        std::string ToString() const
        {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "X=%.3f Y=%.3f", X, Y);
            return buf;
        }

        operator FVector() const { return FVector(static_cast<float>(X), static_cast<float>(Y), static_cast<float>(Z)); }
    };

    inline const FVector3d FVector3d::ZeroVector{0.0, 0.0, 0.0};
}
