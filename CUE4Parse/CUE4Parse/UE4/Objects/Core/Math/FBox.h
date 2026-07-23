// Ported from CUE4Parse/UE4/Objects/Core/Math/FBox.cs — the axis-aligned bounding box.
#pragma once

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#include "FVector.h"
#include "FTransform.h"
#include "Matrix.h"
#include "../../../IUStruct.h"
#include "../../../Readers/FArchive.h"

namespace CUE4Parse::UE4::Objects::Core::Math
{
    using CUE4Parse::UE4::Readers::FArchive;

    struct FBox : public UE4::IUStruct
    {
        /// Holds the box's minimum point.
        FVector Min;
        /// Holds the box's maximum point.
        FVector Max;
        /// Holds a flag indicating whether this box is valid. (A byte, not a bool — that is how it serializes.)
        uint8_t IsValid = 0;

        FBox() = default;
        FBox(FVector min, FVector max, uint8_t isValid = 1) : Min(min), Max(max), IsValid(isValid) {}
        explicit FBox(FArchive& Ar) : Min(Ar), Max(Ar), IsValid(Ar.Read<uint8_t>()) {}

        // Faithful to C#, which sums the points into both Min and Max instead of taking their extremes. That is
        // almost certainly a bug upstream, but nothing in the parse path calls it, and a silent behaviour change
        // here would be a worse surprise than the bug.
        explicit FBox(const std::vector<FVector>& points)
        {
            for (const auto& it : points)
            {
                Min = Min + it;
                Max = Max + it;
            }
        }

        bool Equals(const FBox& other) const { return Min == other.Min && Max == other.Max; }

        friend FBox operator+(const FBox& a, FVector other)
        {
            if (a.IsValid != 0)
            {
                return FBox(
                    FVector(std::min(a.Min.X, other.X), std::min(a.Min.Y, other.Y), std::min(a.Min.Z, other.Z)),
                    FVector(std::max(a.Max.X, other.X), std::max(a.Max.Y, other.Y), std::max(a.Max.Z, other.Z)));
            }
            return FBox(other, other, 1);
        }

        friend FBox operator+(const FBox& a, const FBox& other)
        {
            if (a.IsValid != 0)
            {
                return FBox(
                    FVector(std::min(a.Min.X, other.Min.X), std::min(a.Min.Y, other.Min.Y), std::min(a.Min.Z, other.Min.Z)),
                    FVector(std::max(a.Max.X, other.Max.X), std::max(a.Max.Y, other.Max.Y), std::max(a.Max.Z, other.Max.Z)));
            }
            return FBox(other.Min, other.Max, other.IsValid);
        }

        friend FBox operator*(const FBox& a, float scale)
        {
            return FBox(FVector(a.Min.X * scale, a.Min.Y * scale, a.Min.Z * scale),
                        FVector(a.Max.X * scale, a.Max.Y * scale, a.Max.Z * scale));
        }

        FVector operator[](int i) const
        {
            switch (i) { case 0: return Min; case 1: return Max; default: throw std::out_of_range("FBox index"); }
        }

        /// Calculates the distance of a point to this box.
        float ComputeSquaredDistanceToPoint(FVector point) const
        {
            return FVector::ComputeSquaredDistanceFromBoxToPoint(Min, Max, point);
        }

        /// Increases the box size by w on every axis.
        FBox ExpandBy(float w) const { return FBox(Min - FVector(w, w, w), Max + FVector(w, w, w)); }
        /// Increases the box size by v.
        FBox ExpandBy(FVector v) const { return FBox(Min - v, Max + v); }
        /// Increases the box size outwards by neg in the negative direction and pos in the positive one.
        FBox ExpandBy(FVector neg, FVector pos) const { return FBox(Min - neg, Max + pos); }

        /// Shifts the bounding box position.
        FBox ShiftBy(FVector offset) const { return FBox(Min + offset, Max + offset); }

        /// Moves the center of the bounding box to a new destination.
        FBox MoveTo(FVector destination) const
        {
            const FVector offset = destination - GetCenter();
            return FBox(Min + offset, Max + offset);
        }

        /// Gets the center point of this box.
        FVector GetCenter() const { return (Min + Max) * 0.5f; }

        /// Gets the center and extents of this box.
        void GetCenterAndExtents(FVector& center, FVector& extents) const
        {
            extents = GetExtent();
            center = Min + extents;
        }

        /// Calculates the closest point on or inside the box to a given point in space.
        FVector GetClosestPointTo(FVector point) const
        {
            // start by considering the point inside the box
            FVector closestPoint = point;

            // now clamp to inside box if it's outside
            if (point.X < Min.X) closestPoint.X = Min.X;
            else if (point.X > Max.X) closestPoint.X = Max.X;

            if (point.Y < Min.Y) closestPoint.Y = Min.Y;
            else if (point.Y > Max.Y) closestPoint.Y = Max.Y;

            if (point.Z < Min.Z) closestPoint.Z = Min.Z;
            else if (point.Z > Max.Z) closestPoint.Z = Max.Z;

            return closestPoint;
        }

        FVector GetExtent() const { return (Max - Min) * 0.5f; }
        FVector GetSize() const { return Max - Min; }
        float GetVolume() const { return (Max.X - Min.X) * (Max.Y - Min.Y) * (Max.Z - Min.Z); }

        /// Checks whether the given bounding box intersects this bounding box.
        bool Intersects(const FBox& other) const
        {
            if ((Min.X > other.Max.X) || (other.Min.X > Max.X)) return false;
            if ((Min.Y > other.Max.Y) || (other.Min.Y > Max.Y)) return false;
            if ((Min.Z > other.Max.Z) || (other.Min.Z > Max.Z)) return false;
            return true;
        }

        /// Checks whether the given bounding box intersects this bounding box in the XY plane.
        bool IntersectsXY(const FBox& other) const
        {
            if ((Min.X > other.Max.X) || (other.Min.X > Max.X)) return false;
            if ((Min.Y > other.Max.Y) || (other.Min.Y > Max.Y)) return false;
            return true;
        }

        FBox Overlap(const FBox& other) const
        {
            if (!Intersects(other)) return FBox(FVector(0.0f, 0.0f, 0.0f), FVector(0.0f, 0.0f, 0.0f));

            // otherwise they overlap, so find the overlapping box
            FVector minVector;
            FVector maxVector;

            minVector.X = std::max(Min.X, other.Min.X);
            maxVector.X = std::min(Max.X, other.Max.X);

            minVector.Y = std::max(Min.Y, other.Min.Y);
            maxVector.Y = std::min(Max.Y, other.Max.Y);

            minVector.Z = std::max(Min.Z, other.Min.Z);
            maxVector.Z = std::min(Max.Z, other.Max.Z);

            return FBox(minVector, maxVector);
        }

        /// Checks whether the given location is strictly inside this box.
        bool IsInside(FVector in) const
        {
            return (in.X > Min.X) && (in.X < Max.X) && (in.Y > Min.Y) && (in.Y < Max.Y) &&
                   (in.Z > Min.Z) && (in.Z < Max.Z);
        }

        /// Checks whether the given location is inside or on this box.
        bool IsInsideOrOn(FVector in) const
        {
            return (in.X >= Min.X) && (in.X <= Max.X) && (in.Y >= Min.Y) && (in.Y <= Max.Y) &&
                   (in.Z >= Min.Z) && (in.Z <= Max.Z);
        }

        /// Checks whether a given box is fully encapsulated by this box.
        bool IsInside(const FBox& other) const { return IsInside(other.Min) && IsInside(other.Max); }

        /// Checks whether the given location is inside this box in the XY plane.
        bool IsInsideXY(FVector in) const
        {
            return (in.X > Min.X) && (in.X < Max.X) && (in.Y > Min.Y) && (in.Y < Max.Y);
        }

        /// Checks whether the given box is fully encapsulated by this box in the XY plane.
        bool IsInsideXY(const FBox& other) const { return IsInsideXY(other.Min) && IsInsideXY(other.Max); }

        FBox TransformBy(const FMatrix& m) const
        {
            // if we are not valid, return another invalid box.
            if (IsValid == 0) return FBox();

            const FVector vecMin = Min;
            const FVector vecMax = Max;

            const FVector m0(m.M00, m.M01, m.M02);
            const FVector m1(m.M10, m.M11, m.M12);
            const FVector m2(m.M20, m.M21, m.M22);
            const FVector m3(m.M30, m.M31, m.M32);

            const FVector half(0.5f, 0.5f, 0.5f);
            const FVector origin = (vecMax + vecMin) * half;
            const FVector extent = (vecMax - vecMin) * half;

            const FVector newOrigin = FVector(origin.X) * m0 +
                                      FVector(origin.Y) * m1 +
                                      FVector(origin.Z) * m2 +
                                      m3;

            const FVector newExtent = (FVector(extent.X) * m0).Abs() +
                                      (FVector(extent.Y) * m1).Abs() +
                                      (FVector(extent.Z) * m2).Abs();

            return FBox(newOrigin - newExtent, newOrigin + newExtent, 1);
        }

        FBox TransformBy(const FTransform& m) const { return TransformBy(m.ToMatrixWithScale()); }

        std::string ToString() const
        {
            return std::string("IsValid=") + (IsValid != 0 ? "True" : "False") +
                   ", Min=" + Min.ToString() + ", Max=" + Max.ToString();
        }

        /// Utility function to build an AABB from an origin and a (half-size) extent.
        static FBox BuildAABB(FVector origin, FVector extent) { return FBox(origin - extent, origin + extent); }
    };
}
