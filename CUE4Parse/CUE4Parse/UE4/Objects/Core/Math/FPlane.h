// Ported from CUE4Parse/UE4/Objects/Core/Math/FPlane.cs — a plane stored as (normal.xyz, W).
// USE Ar.Read<FPlane> FOR FLOATS AND FPlane(Ar) FOR DOUBLES (via ReadFReal).
// Deferred: the TIntVector3<float>/<double> ctors (TIntVector3 not ported) and GetHashCode/Equals(object).
#pragma once

#include <cmath>

#include "FVector.h"
#include "UnrealMathUtility.h"
#include "../../../Readers/FArchive.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    using CUE4Parse::UE4::Readers::FArchive;

    // Layout matches the C# [StructLayout(Size = 16)]: an FVector (12 bytes) followed by W.
    struct FPlane
    {
        FVector Vector;
        float W = 0.0f;

        // X/Y/Z alias the packed FVector components (C# exposes them as properties).
        float GetX() const { return Vector.X; }
        float GetY() const { return Vector.Y; }
        float GetZ() const { return Vector.Z; }
        void SetX(float v) { Vector.X = v; }
        void SetY(float v) { Vector.Y = v; }
        void SetZ(float v) { Vector.Z = v; }

        FPlane() = default;
        FPlane(FVector base_, FVector normal) : Vector(base_), W(base_ | normal) {}
        FPlane(float x, float y, float z, float w) : Vector(x, y, z), W(w) {}
        explicit FPlane(FArchive& Ar) : Vector(Ar) { W = Ar.ReadFReal(); }

        float PlaneDot(FVector p) const { return Vector.X * p.X + Vector.Y * p.Y + Vector.Z * p.Z - W; }

        bool Equals(FPlane v, float tolerance) const { return Vector.Equals(v.Vector, tolerance) && std::fabs(W - v.W) <= tolerance; }
        bool Equals(FPlane v) const { return Equals(v, UnrealMath::KindaSmallNumber); }

        friend bool operator==(FPlane a, FPlane b) { return a.Equals(b); }
        friend bool operator!=(FPlane a, FPlane b) { return !a.Equals(b); }
    };
}
