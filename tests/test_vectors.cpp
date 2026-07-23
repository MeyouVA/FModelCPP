// Unit tests for the Core/Math vector layer: FVector arithmetic / geometry, FVector2D / FVector4,
// the integer vectors, packed short/uint vertex formats, and blittable Read<FVector>. Pure value math
// (no package machinery), plus layout static_asserts guaranteeing Ar.Read<T> stays valid.
#include <cmath>
#include <cstring>
#include <iostream>
#include <type_traits>

#include "UE4/Objects/Core/Math/FVector.h"
#include "UE4/Objects/Core/Math/FVector2D.h"
#include "UE4/Objects/Core/Math/FVector4.h"
#include "UE4/Objects/Core/Math/FIntVector.h"
#include "UE4/Objects/Core/Math/FUIntVector.h"
#include "UE4/Objects/Core/Math/FIntPoint.h"
#include "UE4/Objects/Core/Math/FVector3SignedShortScale.h"
#include "UE4/Objects/Core/Math/FTwoVectors.h"
#include "UE4/Readers/FByteArchive.h"

using namespace CUE4Parse::UE4::Objects::Core::Math;
using CUE4Parse::UE4::Readers::FByteArchive;

static int g_failures = 0;
#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::cerr << "FAIL: " << #cond << " (line " << __LINE__ << ")\n";  \
            ++g_failures;                                                      \
        }                                                                      \
    } while (0)

static bool Near(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) < eps; }
static bool NearV(FVector v, float x, float y, float z, float eps = 1e-4f)
{
    return Near(v.X, x, eps) && Near(v.Y, y, eps) && Near(v.Z, z, eps);
}
// Normalization routes through the Quake-III fast InvSqrt (faithful to CUE4Parse), so normalized
// results carry ~0.2% error — compared with a looser tolerance than exact arithmetic.
static constexpr float kNormEps = 3e-3f;

// The blit path (Ar.Read<T>) requires these to stay trivially-copyable + standard-layout.
static_assert(std::is_trivially_copyable_v<FVector> && std::is_standard_layout_v<FVector>);
static_assert(std::is_trivially_copyable_v<FVector4> && std::is_standard_layout_v<FVector4>);
static_assert(std::is_trivially_copyable_v<FVector2D> && std::is_standard_layout_v<FVector2D>);
static_assert(std::is_trivially_copyable_v<FIntVector> && std::is_standard_layout_v<FIntVector>);
static_assert(sizeof(FVector) == 12 && sizeof(FVector4) == 16 && sizeof(FIntVector) == 12);

int main()
{
    // ---------- FVector arithmetic ----------
    CHECK(NearV(FVector(1, 2, 3) + FVector(4, 5, 6), 5, 7, 9));
    CHECK(NearV(FVector(4, 5, 6) - FVector(1, 2, 3), 3, 3, 3));
    CHECK(NearV(FVector(1, 2, 3) * 2.0f, 2, 4, 6));
    CHECK(NearV(2.0f * FVector(1, 2, 3), 2, 4, 6));
    CHECK(NearV(FVector(2, 4, 6) / 2.0f, 1, 2, 3));
    CHECK(NearV(-FVector(1, -2, 3), -1, 2, -3));

    // ---------- dot (|) and cross (^) ----------
    CHECK(Near(FVector(1, 2, 3) | FVector(4, 5, 6), 32.0f));
    CHECK(NearV(FVector(1, 0, 0) ^ FVector(0, 1, 0), 0, 0, 1));
    CHECK(Near(FVector::DotProduct(FVector(1, 2, 3), FVector(4, 5, 6)), 32.0f));
    CHECK(NearV(FVector::CrossProduct(FVector(1, 0, 0), FVector(0, 1, 0)), 0, 0, 1));

    // ---------- geometry ----------
    CHECK(Near(FVector(3, 4, 0).Size(), 5.0f));
    CHECK(Near(FVector(3, 4, 0).SizeSquared(), 25.0f));
    CHECK(Near(FVector(3, 4, 0).Size2D(), 5.0f));
    CHECK(NearV(FVector(3, 4, 0).GetSafeNormal(), 0.6f, 0.8f, 0.0f, kNormEps));
    {
        FVector n(0, 0, 5);
        CHECK(n.Normalize());
        CHECK(NearV(n, 0, 0, 1, kNormEps));
        CHECK(n.IsNormalized()); // 0.01 tolerance; IsUnit (1e-4) is too tight for the approximate InvSqrt
    }
    CHECK(FVector(0, 0, 0).IsZero());
    CHECK(!FVector(1, 0, 0).IsZero());
    CHECK(NearV(FVector(-1, 2, -3).Abs(), 1, 2, 3));
    CHECK(Near(FVector(1, 5, 3).Max(), 5.0f));
    CHECK(Near(FVector(1, 5, 3).Min(), 1.0f));
    CHECK(NearV(FVector(1, 5, 3).ComponentMax(FVector(4, 2, 6)), 4, 5, 6));
    CHECK(NearV(FVector(1, 5, 3).ComponentMin(FVector(4, 2, 6)), 1, 2, 3));

    // ---------- indexer ----------
    {
        FVector v(7, 8, 9);
        CHECK(Near(v[0], 7) && Near(v[1], 8) && Near(v[2], 9));
        v[1] = 42.0f;
        CHECK(Near(v.Y, 42.0f));
        bool threw = false;
        try { (void)v[3]; } catch (const std::out_of_range&) { threw = true; }
        CHECK(threw);
    }

    // ---------- cross-type constructors / conversions ----------
    CHECK(NearV(FVector(FVector4(1, 2, 3, 4)), 1, 2, 3));           // FVector(FVector4)
    CHECK(NearV(static_cast<FVector>(FVector4(1, 2, 3, 4)), 1, 2, 3)); // explicit operator FVector
    {
        FVector4 h(FVector(1, 2, 3), 5.0f);                        // FVector4(FVector, w)
        CHECK(Near(h.X, 1) && Near(h.Y, 2) && Near(h.Z, 3) && Near(h.W, 5));
    }
    CHECK(NearV(FVector(FLinearColor{0.1f, 0.2f, 0.3f, 1.0f}), 0.1f, 0.2f, 0.3f));
    CHECK(NearV(FVector(FIntVector(1, 2, 3)), 1, 2, 3));
    CHECK(NearV(FVector(FIntPoint(4, 5)), 4, 5, 0));
    CHECK(NearV(FVector(FVector2D(1, 2), 3.0f), 1, 2, 3));

    // ---------- FVector2D ----------
    CHECK(Near((FVector2D(1, 2) + FVector2D(3, 4)).X, 4.0f));
    CHECK(Near(FVector2D::ZeroVector.X, 0.0f) && Near(FVector2D::ZeroVector.Y, 0.0f));
    { FVector2D v = FVector2d(1.5, 2.5); CHECK(Near(v.X, 1.5f) && Near(v.Y, 2.5f)); } // FVector2d -> FVector2D

    // ---------- FVector4 ----------
    CHECK(FVector4(1, 2, 3, 4) == FVector4(1, 2, 3, 4));
    CHECK(FVector4(1, 2, 3, 4) != FVector4(1, 2, 3, 5));
    CHECK(FVector4::OneVector.W == 1.0f && FVector4::ZeroVector.X == 0.0f);

    // ---------- integer vectors ----------
    CHECK((FIntVector(1, 2, 3) + FIntVector(4, 5, 6)).X == 5);
    CHECK((FIntVector(1, 2, 3) * 2).Z == 6);
    CHECK(FIntVector::Zero().X == 0);
    CHECK((FUIntVector(1u, 2u, 3u) + 1u).Y == 3u);
    CHECK(FUIntVector(-1, -1, -1).X == 0xFFFFFFFFu);

    // ---------- packed vertex formats -> FVector ----------
    CHECK(NearV(FVector3SignedShortScale(10, 20, 30, 0), 10, 20, 30)); // W==0 -> scale 1
    CHECK(NearV(FVector3SignedShortScale(10, 20, 30, 2), 5, 10, 15));  // W==2 -> /2
    CHECK(NearV(FVector3UnsignedShortScale(3, 4, 5, 1), 3, 4, 5));
    {
        FVector3Packed32 p{ (5u & 0x3ffu) | ((6u & 0x3ffu) << 10) | ((7u & 0x3ffu) << 20) };
        CHECK(NearV(p, 5, 6, 7));
    }

    // ---------- blittable read (Ar.Read<FVector>) ----------
    {
        float src[3] = {1.0f, 2.0f, 3.0f};
        std::vector<uint8_t> bytes(sizeof(src));
        std::memcpy(bytes.data(), src, sizeof(src));
        FByteArchive Ar("vec", bytes);
        FVector v = Ar.Read<FVector>();
        CHECK(NearV(v, 1, 2, 3));
        CHECK(Ar.Position == 12);
    }

    if (g_failures == 0) std::cout << "test_vectors: all checks passed\n";
    return g_failures;
}
