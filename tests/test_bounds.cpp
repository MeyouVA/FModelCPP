// Tests for the Core/Math bounds + ranges cluster: FBox, FBox2D/TBox2, TBox3, FSphere, FBoxSphereBounds,
// TInterval, TRange/TRangeBound, TIntVector/TVector, and the half-precision vertex vectors.
//
// The bounds types are where a sign or a min/max slip stays invisible until a mesh renders inside-out, so the
// checks lean on properties that must hold rather than on hand-computed constants where possible: a transformed
// box must still contain the transformed corners, an accumulated box must contain every point fed into it.
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>

#include "UE4/Objects/Core/Math/FBox.h"
#include "UE4/Objects/Core/Math/FBox2D.h"
#include "UE4/Objects/Core/Math/FBoxSphereBounds.h"
#include "UE4/Objects/Core/Math/FHalfVector.h"
#include "UE4/Objects/Core/Math/FSphere.h"
#include "UE4/Objects/Core/Math/FVector3UnsignedShort.h"
#include "UE4/Objects/Core/Math/FRotator.h"
#include "UE4/Objects/Core/Math/RotationMatrix.h"
#include "UE4/Objects/Core/Math/TBox3.h"
#include "UE4/Objects/Core/Math/TIntVector.h"
#include "UE4/Objects/Core/Math/TInterval.h"
#include "UE4/Objects/Core/Math/TRange.h"
#include "UE4/Readers/FByteArchive.h"

using namespace CUE4Parse::UE4::Objects::Core::Math;
using CUE4Parse::UE4::Readers::FByteArchive;

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL: %s (line %d)\n", #cond, __LINE__); ++g_failures; } } while (0)

static constexpr float kEps = 1e-4f;
static bool Near(float a, float b, float eps = kEps) { return std::fabs(a - b) <= eps; }
static bool NearV(FVector a, float x, float y, float z, float eps = kEps)
{
    return Near(a.X, x, eps) && Near(a.Y, y, eps) && Near(a.Z, z, eps);
}

static std::vector<uint8_t> BytesOf(const void* p, size_t n)
{
    std::vector<uint8_t> b(n);
    std::memcpy(b.data(), p, n);
    return b;
}

int main()
{
    // ---------- FBox basics ----------
    {
        const FBox box(FVector(-1, -2, -3), FVector(4, 5, 6));
        CHECK(box.IsValid == 1);
        CHECK(NearV(box.GetCenter(), 1.5f, 1.5f, 1.5f));
        CHECK(NearV(box.GetExtent(), 2.5f, 3.5f, 4.5f));
        CHECK(NearV(box.GetSize(), 5, 7, 9));
        CHECK(Near(box.GetVolume(), 5 * 7 * 9));

        FVector center, extents;
        box.GetCenterAndExtents(center, extents);
        // GetCenterAndExtents defines the center as Min + extent, which is the same point as GetCenter().
        CHECK(NearV(center, 1.5f, 1.5f, 1.5f));
        CHECK(NearV(extents, 2.5f, 3.5f, 4.5f));

        CHECK(box[0] == box.Min);
        CHECK(box[1] == box.Max);
        bool threw = false;
        try { (void)box[2]; } catch (const std::out_of_range&) { threw = true; }
        CHECK(threw);
    }

    // ---------- containment ----------
    {
        const FBox box(FVector(0, 0, 0), FVector(10, 10, 10));
        CHECK(box.IsInside(FVector(5, 5, 5)));
        CHECK(!box.IsInside(FVector(0, 5, 5)));      // strict: a face point is not "inside"
        CHECK(box.IsInsideOrOn(FVector(0, 5, 5)));
        CHECK(box.IsInsideOrOn(FVector(10, 10, 10)));
        CHECK(!box.IsInsideOrOn(FVector(10.001f, 5, 5)));
        CHECK(box.IsInside(FBox(FVector(1, 1, 1), FVector(9, 9, 9))));
        CHECK(!box.IsInside(FBox(FVector(1, 1, 1), FVector(11, 9, 9))));
        // XY variants ignore Z entirely.
        CHECK(box.IsInsideXY(FVector(5, 5, 999)));
        CHECK(!box.IsInsideXY(FVector(5, 11, 5)));

        CHECK(NearV(box.GetClosestPointTo(FVector(5, 5, 5)), 5, 5, 5));      // already inside
        CHECK(NearV(box.GetClosestPointTo(FVector(-3, 5, 42)), 0, 5, 10));   // clamped on X and Z
        CHECK(Near(box.ComputeSquaredDistanceToPoint(FVector(-3, 5, 5)), 9.0f));
        CHECK(Near(box.ComputeSquaredDistanceToPoint(FVector(5, 5, 5)), 0.0f));
    }

    // ---------- intersection / overlap ----------
    {
        const FBox a(FVector(0, 0, 0), FVector(10, 10, 10));
        const FBox b(FVector(5, 5, 5), FVector(15, 15, 15));
        const FBox far(FVector(20, 20, 20), FVector(30, 30, 30));
        const FBox onlyZapart(FVector(0, 0, 50), FVector(10, 10, 60));

        CHECK(a.Intersects(b));
        CHECK(!a.Intersects(far));
        CHECK(!a.Intersects(onlyZapart));
        CHECK(a.IntersectsXY(onlyZapart)); // XY overlaps even though Z does not
        // Touching faces count as intersecting (the comparisons are strict >).
        CHECK(a.Intersects(FBox(FVector(10, 0, 0), FVector(20, 10, 10))));

        const FBox ov = a.Overlap(b);
        CHECK(NearV(ov.Min, 5, 5, 5));
        CHECK(NearV(ov.Max, 10, 10, 10));
        const FBox none = a.Overlap(far);
        CHECK(NearV(none.Min, 0, 0, 0) && NearV(none.Max, 0, 0, 0));
    }

    // ---------- expand / shift / move ----------
    {
        const FBox box(FVector(0, 0, 0), FVector(10, 10, 10));
        const FBox e1 = box.ExpandBy(2.0f);
        CHECK(NearV(e1.Min, -2, -2, -2) && NearV(e1.Max, 12, 12, 12));
        const FBox e2 = box.ExpandBy(FVector(1, 2, 3));
        CHECK(NearV(e2.Min, -1, -2, -3) && NearV(e2.Max, 11, 12, 13));
        // Both arguments push outwards, so the negative side subtracts.
        const FBox e3 = box.ExpandBy(FVector(1, 1, 1), FVector(5, 0, 0));
        CHECK(NearV(e3.Min, -1, -1, -1) && NearV(e3.Max, 15, 10, 10));

        const FBox s = box.ShiftBy(FVector(1, 2, 3));
        CHECK(NearV(s.Min, 1, 2, 3) && NearV(s.Max, 11, 12, 13));

        const FBox m = box.MoveTo(FVector(0, 0, 0));
        CHECK(NearV(m.GetCenter(), 0, 0, 0));
        CHECK(NearV(m.GetExtent(), 5, 5, 5));

        CHECK(NearV((box * 2.0f).Max, 20, 20, 20));

        const FBox aabb = FBox::BuildAABB(FVector(1, 1, 1), FVector(2, 3, 4));
        CHECK(NearV(aabb.Min, -1, -2, -3) && NearV(aabb.Max, 3, 4, 5));
    }

    // ---------- accumulation operators ----------
    {
        // An invalid box takes on the first point rather than growing from the origin.
        FBox acc;
        CHECK(acc.IsValid == 0);
        acc = acc + FVector(5, 5, 5);
        CHECK(acc.IsValid == 1);
        CHECK(NearV(acc.Min, 5, 5, 5) && NearV(acc.Max, 5, 5, 5));

        const FVector points[] = {FVector(5, 5, 5), FVector(-2, 9, 1), FVector(7, 0, -4)};
        for (const auto& p : points) acc = acc + p;
        CHECK(NearV(acc.Min, -2, 0, -4));
        CHECK(NearV(acc.Max, 7, 9, 5));
        for (const auto& p : points) CHECK(acc.IsInsideOrOn(p)); // the property that matters

        FBox invalid;
        const FBox valid(FVector(1, 1, 1), FVector(2, 2, 2));
        const FBox merged = invalid + valid;
        CHECK(merged.IsValid == 1 && NearV(merged.Min, 1, 1, 1) && NearV(merged.Max, 2, 2, 2));
        const FBox union2 = valid + FBox(FVector(-1, 0, 0), FVector(1, 5, 1));
        CHECK(NearV(union2.Min, -1, 0, 0) && NearV(union2.Max, 2, 5, 2));
    }

    // ---------- TransformBy ----------
    {
        // An invalid box stays invalid however it is transformed.
        CHECK(FBox().TransformBy(FMatrix::Identity()).IsValid == 0);

        const FBox box(FVector(-1, -1, -1), FVector(1, 1, 1));
        CHECK(box.TransformBy(FMatrix::Identity()).Equals(box));

        // 90 degrees about Z maps the unit cube onto itself.
        const FMatrix rot = FRotationMatrix(FRotator(0.0f, 90.0f, 0.0f));
        const FBox rotated = box.TransformBy(rot);
        CHECK(rotated.IsValid == 1);
        CHECK(NearV(rotated.Min, -1, -1, -1, 1e-3f));
        CHECK(NearV(rotated.Max, 1, 1, 1, 1e-3f));

        // A rotated *non*-cubic box must still contain every transformed corner: that is the invariant the
        // Abs() terms in TransformBy exist to guarantee.
        const FBox oblong(FVector(-3, -1, -0.5f), FVector(3, 1, 0.5f));
        const FMatrix odd = FRotationMatrix(FRotator(20.0f, 35.0f, 15.0f));
        const FBox oblongT = oblong.TransformBy(odd);
        for (int i = 0; i < 8; ++i)
        {
            const FVector corner((i & 1) ? oblong.Max.X : oblong.Min.X,
                                 (i & 2) ? oblong.Max.Y : oblong.Min.Y,
                                 (i & 4) ? oblong.Max.Z : oblong.Min.Z);
            CHECK(oblongT.ExpandBy(1e-3f).IsInsideOrOn(static_cast<FVector>(odd.TransformPosition(corner))));
        }
    }

    // ---------- FSphere ----------
    {
        const FSphere s(FVector(1, 2, 3), 4.0f);
        CHECK(NearV(s.Center, 1, 2, 3) && Near(s.W, 4.0f));
        const FSphere scaled = s * 2.0f;
        CHECK(NearV(scaled.Center, 2, 4, 6) && Near(scaled.W, 8.0f));
        CHECK(NearV(FSphere(1, 2, 3, 4).Center, 1, 2, 3));
        CHECK(NearV(FSphere(TIntVector3<float>(1, 2, 3), 4.0f).Center, 1, 2, 3));
        CHECK(NearV(FSphere(TIntVector3<double>(1, 2, 3), 4.0).Center, 1, 2, 3));
    }

    // ---------- FBoxSphereBounds ----------
    {
        const FBox box(FVector(-1, -2, -3), FVector(1, 2, 3));
        const FBoxSphereBounds b(box);
        CHECK(NearV(b.Origin, 0, 0, 0));
        CHECK(NearV(b.BoxExtent, 1, 2, 3));
        CHECK(Near(b.SphereRadius, std::sqrt(1.0f + 4.0f + 9.0f)));
        // GetBox() reverses the construction.
        CHECK(b.GetBox().Equals(box));

        const FBoxSphereBounds fromSphere{FSphere(FVector(1, 1, 1), 5.0f)};
        CHECK(NearV(fromSphere.Origin, 1, 1, 1));
        CHECK(NearV(fromSphere.BoxExtent, 5, 5, 5));
        CHECK(Near(fromSphere.SphereRadius, 5.0f));

        // The box+sphere ctor takes the tighter of the two radii.
        const FBoxSphereBounds both(box, FSphere(FVector(0, 0, 0), 0.5f));
        CHECK(Near(both.SphereRadius, 0.5f));

        // Under a pure rotation the radius is preserved and the origin follows the rotation.
        const FMatrix rot = FRotationMatrix(FRotator(0.0f, 90.0f, 0.0f));
        const FBoxSphereBounds rotated = FBoxSphereBounds(FVector(1, 0, 0), FVector(1, 1, 1), 2.0f).TransformBy(rot);
        CHECK(Near(rotated.SphereRadius, std::sqrt(3.0f), 1e-3f)); // clamped by the box-extent magnitude
        CHECK(NearV(rotated.Origin, 0, 1, 0, 1e-3f));

        // Under a uniform scale of 3 the radius scales by 3 (the box magnitude scales too, so no clamping).
        FMatrix scale3 = FMatrix::Identity();
        scale3.M00 = scale3.M11 = scale3.M22 = 3.0f;
        const FBoxSphereBounds scaled = FBoxSphereBounds(FVector(1, 0, 0), FVector(1, 1, 1), 1.0f).TransformBy(scale3);
        CHECK(NearV(scaled.Origin, 3, 0, 0));
        CHECK(NearV(scaled.BoxExtent, 3, 3, 3));
        CHECK(Near(scaled.SphereRadius, 3.0f, 1e-3f));
    }

    // ---------- archive reads ----------
    {
        // FBox: 6 floats + the validity byte.
        const float src[6] = {-1, -2, -3, 4, 5, 6};
        std::vector<uint8_t> bytes = BytesOf(src, sizeof(src));
        bytes.push_back(1);
        FByteArchive Ar("box", bytes);
        const FBox box(Ar);
        CHECK(NearV(box.Min, -1, -2, -3) && NearV(box.Max, 4, 5, 6));
        CHECK(box.IsValid == 1);
        CHECK(Ar.Position == 25);
    }
    {
        const float src[4] = {1, 2, 3, 4};
        std::vector<uint8_t> bytes = BytesOf(src, sizeof(src));
        bytes.push_back(1);
        FByteArchive Ar("box2d", bytes);
        const FBox2D box(Ar);
        CHECK(Near(box.Min.X, 1) && Near(box.Min.Y, 2));
        CHECK(Near(box.Max.X, 3) && Near(box.Max.Y, 4));
        CHECK(box.bIsValid == 1);
        CHECK(Ar.Position == 17);
    }
    {
        // TBox3<int32_t>: Min and Max must come off the wire in declaration order.
        const int32_t src[6] = {1, 2, 3, 40, 50, 60};
        std::vector<uint8_t> bytes = BytesOf(src, sizeof(src));
        bytes.push_back(1);
        FByteArchive Ar("tbox3", bytes);
        const TBox3<int32_t> box(Ar);
        CHECK(box.Min.X == 1 && box.Min.Y == 2 && box.Min.Z == 3);
        CHECK(box.Max.X == 40 && box.Max.Y == 50 && box.Max.Z == 60);
        CHECK(box.bIsValid == 1);
        CHECK(Ar.Position == 25);
    }
    {
        const int16_t src[4] = {1, 2, 30, 40};
        std::vector<uint8_t> bytes = BytesOf(src, sizeof(src));
        bytes.push_back(0);
        FByteArchive Ar("tbox2", bytes);
        const TBox2<int16_t> box(Ar);
        CHECK(box.Min.X == 1 && box.Min.Y == 2);
        CHECK(box.Max.X == 30 && box.Max.Y == 40);
        CHECK(box.bIsValid == 0);
        CHECK(Ar.Position == 9);
    }
    {
        // FSphere reads its radius only past UE3 version 61; the archive defaults well past that.
        const float src[4] = {1, 2, 3, 7};
        FByteArchive Ar("sphere", BytesOf(src, sizeof(src)));
        const FSphere s(Ar);
        CHECK(NearV(s.Center, 1, 2, 3) && Near(s.W, 7.0f));
    }
    {
        const float src[7] = {1, 2, 3, 4, 5, 6, 7};
        FByteArchive Ar("bounds", BytesOf(src, sizeof(src)));
        const FBoxSphereBounds b(Ar);
        CHECK(NearV(b.Origin, 1, 2, 3));
        CHECK(NearV(b.BoxExtent, 4, 5, 6));
        CHECK(Near(b.SphereRadius, 7.0f));
        CHECK(Ar.Position == 28); // the "(28 bytes)" the C# doc comment advertises
    }

    // ---------- TIntVector / TVector ----------
    {
        CHECK(TIntVector1<int>(7).Value == 7);
        CHECK(TIntVector2<int>(1, 2).ToString() == "X: 1, Y: 2");
        CHECK(TIntVector3<int>(1, 2, 3).ToString() == "X: 1, Y: 2, Z: 3");
        CHECK(TIntVector4<int>(1, 2, 3, 4).ToString() == "X: 1, Y: 2, Z: 3, W: 4");

        TVector<int> filled(3, 9);
        CHECK(filled.Dimension() == 3 && filled[0] == 9 && filled[2] == 9);
        bool threw = false;
        try { (void)filled[3]; } catch (const std::out_of_range&) { threw = true; }
        CHECK(threw);

        const float src[4] = {1.5f, 2.5f, 3.5f, 4.5f};
        FByteArchive Ar("tvec", BytesOf(src, sizeof(src)));
        const TVector<float> read(Ar, 4);
        CHECK(read.Dimension() == 4 && Near(read[0], 1.5f) && Near(read[3], 4.5f));
        CHECK(Ar.Position == 16);
    }

    // ---------- TInterval / TRange ----------
    {
        const TInterval<float> iv(1.0f, 2.0f);
        CHECK(Near(iv.Min, 1.0f) && Near(iv.Max, 2.0f));
        CHECK(iv.ToString().rfind("Min: 1.", 0) == 0);

        const TRange<float> r(TRangeBound<float>(ERangeBoundTypes::Inclusive, 0.0f),
                              TRangeBound<float>(ERangeBoundTypes::Exclusive, 10.0f));
        CHECK(r.LowerBound.Type == ERangeBoundTypes::Inclusive);
        CHECK(r.UpperBound.Type == ERangeBoundTypes::Exclusive);
        CHECK(Near(r.UpperBound.Value, 10.0f));
        // Packed, so the bound is 5 bytes and the range 10 — the layout it is read with.
        CHECK(sizeof(TRangeBound<float>) == 5);
        CHECK(sizeof(TRange<float>) == 10);
    }

    // ---------- half-precision vectors ----------
    {
        // Known IEEE-754 binary16 encodings.
        CHECK(Near(FFloat16{}.ToFloat(), 0.0f));
        FFloat16 one; one.Encoded = 0x3C00; CHECK(Near(one.ToFloat(), 1.0f));
        FFloat16 minusTwo; minusTwo.Encoded = 0xC000; CHECK(Near(minusTwo.ToFloat(), -2.0f));
        FFloat16 half; half.Encoded = 0x3800; CHECK(Near(half.ToFloat(), 0.5f));
        FFloat16 smallest; smallest.Encoded = 0x0001; // the smallest subnormal, 2^-24
        CHECK(Near(smallest.ToFloat(), 5.9604645e-8f, 1e-12f));
        FFloat16 maxSub; maxSub.Encoded = 0x03FF;
        CHECK(Near(maxSub.ToFloat(), 6.0975552e-5f, 1e-9f));
        FFloat16 inf; inf.Encoded = 0x7C00; CHECK(std::isinf(inf.ToFloat()) && inf.ToFloat() > 0);
        FFloat16 nan; nan.Encoded = 0x7E00; CHECK(std::isnan(nan.ToFloat()));

        // FromFloat is the exact inverse on values a half can hold.
        const float exact[] = {0.0f, 1.0f, -1.0f, 0.5f, -2.0f, 65504.0f, 5.9604645e-8f, 6.0975552e-5f};
        for (float v : exact) CHECK(Near(FFloat16::FromFloat(v).ToFloat(), v, std::fabs(v) * 1e-6f + 1e-12f));
        CHECK(FFloat16::FromFloat(1.0f).Encoded == 0x3C00);
        CHECK(FFloat16::FromFloat(-2.0f).Encoded == 0xC000);
        CHECK(std::isinf(FFloat16::FromFloat(1e30f).ToFloat()));    // overflow
        CHECK(Near(FFloat16::FromFloat(1e-30f).ToFloat(), 0.0f));   // underflow
        CHECK(std::isnan(FFloat16::FromFloat(std::nanf("")).ToFloat()));

        const FFloat16 h1 = FFloat16::FromFloat(1.0f);
        const FFloat16 h2 = FFloat16::FromFloat(2.0f);
        const FFloat16 h3 = FFloat16::FromFloat(3.0f);
        CHECK(NearV(FHalfVector(h1, h2, h3).ToFVector(), 1, 2, 3));
        CHECK(NearV(FHalfVector4(h1, h2, h3, h2).ToFVector(), 2, 4, 6));     // W scales
        CHECK(NearV(FHalfVectorScaled(h1, h2, h3, h2).ToFVector(), 2, 4, 6));
        CHECK(NearV(FHalfVectorScaled(h1, h2, h3, FFloat16()).ToFVector(), 1, 2, 3)); // W==0 means scale 1
        CHECK(NearV(FHalfVector(h1, h2, h3).ToScaled().ToFVector(), 1, 2, 3));
        CHECK(NearV(FVector3UnsignedShort(h1, h2, h3, h1).ToFVector(), 1, 2, 3)); // W dropped

        // Wire layouts: four halves, tightly packed.
        CHECK(sizeof(FHalfVector) == 6);
        CHECK(sizeof(FHalfVector4) == 8);
        CHECK(sizeof(FHalfVectorScaled) == 8);
        CHECK(sizeof(FVector3UnsignedShort) == 8);
        {
            const uint16_t src[4] = {0x3C00, 0x4000, 0x4200, 0x3C00};
            FByteArchive Ar("half", BytesOf(src, sizeof(src)));
            const auto v = Ar.Read<FHalfVector4>();
            CHECK(NearV(v.ToFVector(), 1, 2, 3));
        }
    }

    if (g_failures == 0) std::cout << "test_bounds: all checks passed\n";
    return g_failures;
}
