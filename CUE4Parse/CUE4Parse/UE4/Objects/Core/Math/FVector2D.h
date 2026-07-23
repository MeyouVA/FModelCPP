// Ported from CUE4Parse/UE4/Objects/Core/Math/FVector2D.cs.
// USE Ar.Read<FVector2D> FOR FLOATS AND FVector2D(Ar) FOR DOUBLES (via ReadFReal).
// The System.Numerics Vector2 conversion is deferred (no such type here).
#pragma once

#include <cstdio>
#include <string>

#include "../../../Readers/FArchive.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    using CUE4Parse::UE4::Readers::FArchive;

    struct FVector2D
    {
        float X = 0.0f;
        float Y = 0.0f;

        static const FVector2D ZeroVector;

        FVector2D() = default;
        FVector2D(float x, float y) : X(x), Y(y) {}
        explicit FVector2D(FArchive& Ar) { X = Ar.ReadFReal(); Y = Ar.ReadFReal(); }

        friend FVector2D operator+(FVector2D a, FVector2D b) { return {a.X + b.X, a.Y + b.Y}; }
        friend FVector2D operator-(FVector2D a, FVector2D b) { return {a.X - b.X, a.Y - b.Y}; }
        friend FVector2D operator*(FVector2D a, FVector2D b) { return {a.X * b.X, a.Y * b.Y}; }
        friend FVector2D operator/(FVector2D a, FVector2D b) { return {a.X / b.X, a.Y / b.Y}; }
        friend FVector2D operator+(FVector2D a, float b) { return {a.X + b, a.Y + b}; }
        friend FVector2D operator*(FVector2D a, float b) { return {a.X * b, a.Y * b}; }
        friend FVector2D operator-(FVector2D a, float b) { return {a.X - b, a.Y - b}; }

        std::string ToString() const
        {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "X=%.3f Y=%.3f", X, Y);
            return buf;
        }
    };

    inline const FVector2D FVector2D::ZeroVector{0, 0};

    // The double-precision sibling (FVector2d in C#).
    struct FVector2d
    {
        double X = 0.0;
        double Y = 0.0;

        static const FVector2d ZeroVector;

        FVector2d() = default;
        FVector2d(float x, float y) : X(x), Y(y) {}
        FVector2d(double x, double y) : X(x), Y(y) {}
        explicit FVector2d(FArchive& Ar) { X = Ar.ReadFReal(); Y = Ar.ReadFReal(); }

        friend FVector2d operator+(FVector2d a, FVector2d b) { return {a.X + b.X, a.Y + b.Y}; }
        friend FVector2d operator-(FVector2d a, FVector2d b) { return {a.X - b.X, a.Y - b.Y}; }
        friend FVector2d operator*(FVector2d a, FVector2d b) { return {a.X * b.X, a.Y * b.Y}; }
        friend FVector2d operator/(FVector2d a, FVector2d b) { return {a.X / b.X, a.Y / b.Y}; }
        friend FVector2d operator+(FVector2d a, float b) { return {a.X + b, a.Y + b}; }
        friend FVector2d operator*(FVector2d a, float b) { return {a.X * b, a.Y * b}; }
        friend FVector2d operator-(FVector2d a, float b) { return {a.X - b, a.Y - b}; }

        std::string ToString() const
        {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "X=%.3f Y=%.3f", X, Y);
            return buf;
        }

        // implicit operator FVector2D(FVector2d)
        operator FVector2D() const { return FVector2D(static_cast<float>(X), static_cast<float>(Y)); }
    };

    inline const FVector2d FVector2d::ZeroVector{0.0, 0.0};
}
