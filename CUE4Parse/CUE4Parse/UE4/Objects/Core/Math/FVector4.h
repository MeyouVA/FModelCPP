// Ported from CUE4Parse/UE4/Objects/Core/Math/FVector4.cs — a 4D homogeneous vector, 4x1 floats.
// USE Ar.Read<FVector4> FOR FLOATS AND FVector4(Ar) FOR DOUBLES (via ReadFReal).
// Deferred: the System.Numerics Vector4 conversion and AsFVector (Unsafe.As reinterpret).
#pragma once

#include <cstdio>
#include <string>

#include "FLinearColor.h"
#include "../../../Readers/FArchive.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    using CUE4Parse::UE4::Readers::FArchive;

    struct FVector; // defined in FVector.h; the cross-type members below are defined in FVector.cpp

    struct FVector4
    {
        float X = 0.0f;
        float Y = 0.0f;
        float Z = 0.0f;
        float W = 0.0f;

        static const FVector4 ZeroVector;
        static const FVector4 OneVector;

        FVector4() = default;
        FVector4(float x, float y, float z, float w) : X(x), Y(y), Z(z), W(w) {}
        explicit FVector4(float x) : FVector4(x, x, x, x) {}
        explicit FVector4(FArchive& Ar) { X = Ar.ReadFReal(); Y = Ar.ReadFReal(); Z = Ar.ReadFReal(); W = Ar.ReadFReal(); }

        // Constructor from a 3D vector and W (defined in FVector.cpp, where FVector is complete).
        explicit FVector4(const FVector& v, float w = 1.0f);

        FVector4(const FLinearColor& color) : X(color.R), Y(color.G), Z(color.B), W(color.A) {}

        // explicit operator FVector (drops W); defined in FVector.cpp.
        explicit operator FVector() const;

        friend bool operator==(const FVector4& a, const FVector4& b)
        {
            return a.X == b.X && a.Y == b.Y && a.Z == b.Z && a.W == b.W;
        }
        friend bool operator!=(const FVector4& a, const FVector4& b) { return !(a == b); }

        bool Equals(const FVector4& other) const { return *this == other; }

        std::string ToString() const
        {
            char buf[96];
            std::snprintf(buf, sizeof(buf), "X=%.3f Y=%.3f Z=%.3f W=%.3f", X, Y, Z, W);
            return buf;
        }
    };

    inline const FVector4 FVector4::ZeroVector{0, 0, 0, 0};
    inline const FVector4 FVector4::OneVector{1, 1, 1, 1};
}
