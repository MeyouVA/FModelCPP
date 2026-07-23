// Tests for the Core/Math geometry cluster: FRotator, FQuat, FMatrix (+ the rotation-matrix helpers),
// FTransform and FPlane. The three representations of a rotation (rotator / quaternion / matrix) are
// cross-checked against each other, which is the property that actually matters for mesh + skeleton parsing.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <type_traits>

#include "UE4/Objects/Core/Math/FPlane.h"
#include "UE4/Objects/Core/Math/FQuat.h"
#include "UE4/Objects/Core/Math/FRotator.h"
#include "UE4/Objects/Core/Math/FTransform.h"
#include "UE4/Objects/Core/Math/FVector.h"
#include "UE4/Objects/Core/Math/Matrix.h"
#include "UE4/Objects/Core/Math/QuatRotationTranslationMatrix.h"
#include "UE4/Objects/Core/Math/RotationMatrix.h"

using namespace CUE4Parse::UE4::Objects::Core::Math;

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL: %s (line %d)\n", #cond, __LINE__); ++g_failures; } } while (0)

static constexpr float kEps = 1e-4f;
// Anything that runs through Normalize() picks up the error of the Quake III InvSqrt approximation
// (~0.2%), which the C# source uses bit-for-bit as well.
static constexpr float kNormEps = 3e-3f;

static bool Near(float a, float b, float eps = kEps) { return std::fabs(a - b) <= eps; }
static bool NearV(FVector a, FVector b, float eps = kEps)
{
    return Near(a.X, b.X, eps) && Near(a.Y, b.Y, eps) && Near(a.Z, b.Z, eps);
}
// Comparisons scaled by the magnitude of the expected value. Anything that passes through Normalize()
// inherits InvSqrt's ~0.17% relative error, so a fixed epsilon is the wrong instrument once the vectors
// being compared are long (a scaled transform easily reaches a magnitude of 40).
static bool NearRelV(FVector a, FVector b, float rel)
{
    const float eps = rel * std::max(1.0f, b.Size());
    return NearV(a, b, eps);
}
static bool NearQ(const FQuat& a, const FQuat& b, float eps = kEps)
{
    const bool same = Near(a.X, b.X, eps) && Near(a.Y, b.Y, eps) && Near(a.Z, b.Z, eps) && Near(a.W, b.W, eps);
    // q and -q are the same rotation.
    const bool flipped = Near(a.X, -b.X, eps) && Near(a.Y, -b.Y, eps) && Near(a.Z, -b.Z, eps) && Near(a.W, -b.W, eps);
    return same || flipped;
}

static const float kPi = CUE4Parse::Utils::MathConstants::PI_F;

static void TestRotator()
{
    CHECK(FRotator::ZeroRotator.Pitch == 0 && FRotator::ZeroRotator.Yaw == 0 && FRotator::ZeroRotator.Roll == 0);

    // ClampAxis -> [0,360), NormalizeAxis -> (-180,180].
    CHECK(Near(FRotator::ClampAxis(370.0f), 10.0f));
    CHECK(Near(FRotator::ClampAxis(-10.0f), 350.0f));
    CHECK(Near(FRotator::NormalizeAxis(190.0f), -170.0f));
    CHECK(Near(FRotator::NormalizeAxis(-190.0f), 170.0f));
    CHECK(Near(FRotator::NormalizeAxis(180.0f), 180.0f));

    FRotator r(10.0f, 400.0f, -190.0f);
    r.Normalize();
    CHECK(Near(r.Pitch, 10.0f) && Near(r.Yaw, 40.0f) && Near(r.Roll, 170.0f));

    // Arithmetic.
    const FRotator a(10.0f, 20.0f, 30.0f);
    const FRotator b(1.0f, 2.0f, 3.0f);
    CHECK((a + b).Yaw == 22.0f);
    CHECK((a - b).Roll == 27.0f);
    CHECK((a * 2.0f).Pitch == 20.0f);

    // Yaw of 90 degrees points along +Y.
    CHECK(NearV(FRotator(0.0f, 90.0f, 0.0f).Vector(), FVector(0.0f, 1.0f, 0.0f)));
    CHECK(NearV(FRotator::ZeroRotator.Vector(), FVector(1.0f, 0.0f, 0.0f)));

    // Byte / short axis compression round-trips (lossy, so only to within a quantum).
    CHECK(Near(FRotator::DecompressAxisFromByte(FRotator::CompressAxisToByte(90.0f)), 90.0f, 360.0f / 256.0f));
    CHECK(Near(FRotator::DecompressAxisFromShort(FRotator::CompressAxisToShort(3.0f)), 3.0f, 360.0f / 65536.0f));

    // Equals uses the shortest angular distance, so 359 and -1 are the same rotation.
    CHECK(FRotator(0.0f, 359.0f, 0.0f).Equals(FRotator(0.0f, -1.0f, 0.0f)));
    CHECK(FRotator(1.0f, 2.0f, 3.0f) == FRotator(1.0f, 2.0f, 3.0f));
    CHECK(FRotator(1.0f, 2.0f, 3.0f) != FRotator(1.0f, 2.0f, 4.0f));
}

static void TestQuat()
{
    CHECK(FQuat::Identity.X == 0 && FQuat::Identity.Y == 0 && FQuat::Identity.Z == 0 && FQuat::Identity.W == 1);
    CHECK(FQuat::Identity.IsIdentity());
    CHECK(FQuat::Identity.IsNormalized());
    CHECK(FQuat(EForceInit::ForceInit).W == 1.0f);
    CHECK(FQuat(EForceInit::ForceInitToZero).W == 0.0f);

    // 90 degrees about +Z takes +X to +Y and +Y to -X.
    const FQuat qz(FVector(0.0f, 0.0f, 1.0f), kPi / 2.0f);
    CHECK(qz.IsNormalized());
    CHECK(Near(qz.Size(), 1.0f));
    CHECK(NearV(qz.RotateVector(FVector(1.0f, 0.0f, 0.0f)), FVector(0.0f, 1.0f, 0.0f)));
    CHECK(NearV(qz.RotateVector(FVector(0.0f, 1.0f, 0.0f)), FVector(-1.0f, 0.0f, 0.0f)));
    CHECK(NearV(qz * FVector(1.0f, 0.0f, 0.0f), FVector(0.0f, 1.0f, 0.0f))); // operator* == RotateVector

    // UnrotateVector and Inverse both undo the rotation.
    CHECK(NearV(qz.UnrotateVector(qz.RotateVector(FVector(1.0f, 2.0f, 3.0f))), FVector(1.0f, 2.0f, 3.0f)));
    CHECK(NearV(qz.Inverse().RotateVector(qz.RotateVector(FVector(1.0f, 2.0f, 3.0f))), FVector(1.0f, 2.0f, 3.0f)));

    // Composition: (b * a) applies a first, then b. Two 90-degree turns about Z make a 180.
    const FQuat twice = qz * qz;
    CHECK(NearV(twice.RotateVector(FVector(1.0f, 0.0f, 0.0f)), FVector(-1.0f, 0.0f, 0.0f)));

    const FQuat qy(FVector(0.0f, 1.0f, 0.0f), kPi / 3.0f);
    const FVector p(0.3f, -0.7f, 1.1f);
    CHECK(NearV((qy * qz).RotateVector(p), qy.RotateVector(qz.RotateVector(p))));

    // Conjugate / static Conjugate.
    FQuat c = qz;
    c.Conjugate();
    CHECK(NearQ(c, FQuat::Conjugate(qz)));
    CHECK(NearQ(c, qz.Inverse())); // conjugate == inverse for a unit quaternion

    // Dot product and scaling.
    CHECK(Near(qz | qz, 1.0f));
    CHECK(Near((qz * 2.0f).SizeSquared(), 4.0f));
    CHECK(Near((FQuat::Identity + FQuat::Identity).W, 2.0f));

    // Normalize: a scaled quaternion renormalizes, a degenerate one falls back to identity.
    FQuat scaled(0.0f, 0.0f, 3.0f, 3.0f);
    scaled.Normalize();
    CHECK(Near(scaled.Size(), 1.0f, kNormEps));
    FQuat degenerate(0.0f, 0.0f, 0.0f, 0.0f);
    degenerate.Normalize();
    CHECK(degenerate.IsIdentity());

    // Slerp endpoints, and the halfway point of a 90-degree turn is a 45-degree turn.
    CHECK(NearQ(FQuat::Slerp(FQuat::Identity, qz, 0.0f), FQuat::Identity, kNormEps));
    CHECK(NearQ(FQuat::Slerp(FQuat::Identity, qz, 1.0f), qz, kNormEps));
    CHECK(NearQ(FQuat::Slerp(FQuat::Identity, qz, 0.5f), FQuat(FVector(0.0f, 0.0f, 1.0f), kPi / 4.0f), kNormEps));
    CHECK(NearQ(FQuat::FastLerp(FQuat::Identity, FQuat::Identity, 0.5f), FQuat::Identity));

    // FindBetweenNormals produces the rotation taking a to b.
    const FVector from(1.0f, 0.0f, 0.0f);
    const FVector to(0.0f, 0.0f, 1.0f);
    CHECK(NearV(FQuat::FindBetweenNormals(from, to).RotateVector(from), to, kNormEps));

    CHECK(!FQuat::Identity.ContainsNaN());
    CHECK(FQuat::Identity.IsVectorZero());
    CHECK(FQuat::Identity == FQuat(0.0f, 0.0f, 0.0f, 1.0f));
    CHECK(FQuat::Identity != qz);
}

static void TestRotatorQuatRoundTrip()
{
    // A rotator -> quaternion -> rotator round-trip must land back on the same orientation, and the
    // quaternion must rotate vectors the same way the rotator's own matrix does.
    const FRotator rotators[] = {
        FRotator(0.0f, 90.0f, 0.0f),
        FRotator(30.0f, 0.0f, 0.0f),
        FRotator(0.0f, 0.0f, 45.0f),
        FRotator(20.0f, -35.0f, 60.0f),
        FRotator(-10.0f, 170.0f, -80.0f),
    };

    const FVector v(0.4f, -1.3f, 2.0f);
    for (const FRotator& r : rotators)
    {
        const FQuat q = r.Quaternion();
        CHECK(q.IsNormalized());
        CHECK(NearQ(q, FQuat(r)));                       // the FQuat(FRotator) ctor agrees
        CHECK(r.GetNormalized().Equals(q.Rotator(), 0.05f)); // quat -> rotator returns the same orientation
        CHECK(NearV(q.RotateVector(v), r.RotateVector(v), 1e-3f));
        CHECK(NearV(q.UnrotateVector(v), r.UnrotateVector(v), 1e-3f));
    }
}

static void TestMatrix()
{
    const FMatrix id = FMatrix::Identity();
    CHECK(id.M00 == 1 && id.M11 == 1 && id.M22 == 1 && id.M33 == 1);
    CHECK(id.M01 == 0 && id.M30 == 0);
    CHECK(Near(id.Determinant(), 1.0f));
    CHECK(Near(id.RotDeterminant(), 1.0f));

    // Default construction is the zero matrix (C# `new FMatrix()`).
    const FMatrix zero;
    CHECK(zero.M00 == 0 && zero.M33 == 0);

    // Identity is the multiplicative identity.
    const FMatrix m(
        1, 2, 3, 0,
        4, 5, 6, 0,
        7, 8, 10, 0,
        11, 12, 13, 1);
    const FMatrix mi = m * id;
    CHECK(Near(mi.M00, m.M00) && Near(mi.M21, m.M21) && Near(mi.M32, m.M32));

    // Transpose is an involution and swaps off-diagonals.
    CHECK(Near(m.GetTransposed().M01, m.M10));
    CHECK(Near(m.GetTransposed().GetTransposed().M12, m.M12));

    // Indexer covers the 16 slots in row-major order and rejects the rest.
    CHECK(m[0] == m.M00 && m[5] == m.M11 && m[14] == m.M32 && m[15] == m.M33);
    FMatrix mutableM = m;
    mutableM[6] = 42.0f;
    CHECK(mutableM.M12 == 42.0f);
    bool threw = false;
    try { (void) m[16]; } catch (const std::out_of_range&) { threw = true; }
    CHECK(threw);

    // Axis / origin accessors.
    CHECK(NearV(m.GetScaledAxis(EAxis::X), FVector(1, 2, 3)));
    CHECK(NearV(m.GetScaledAxis(EAxis::Y), FVector(4, 5, 6)));
    CHECK(NearV(m.GetScaledAxis(EAxis::Z), FVector(7, 8, 10)));
    CHECK(NearV(m.GetScaledAxis(EAxis::None), FVector::ZeroVector));
    CHECK(NearV(m.GetOrigin(), FVector(11, 12, 13)));

    FMatrix axisSet = FMatrix::Identity();
    axisSet.SetAxis(3, FVector(5, 6, 7));
    CHECK(NearV(axisSet.GetOrigin(), FVector(5, 6, 7)));

    // Inverse: M * M^-1 == Identity.
    const FMatrix inv = m.InverseFast();
    const FMatrix shouldBeId = m * inv;
    CHECK(Near(shouldBeId.M00, 1.0f, 1e-3f) && Near(shouldBeId.M11, 1.0f, 1e-3f) &&
          Near(shouldBeId.M22, 1.0f, 1e-3f) && Near(shouldBeId.M33, 1.0f, 1e-3f));
    CHECK(Near(shouldBeId.M01, 0.0f, 1e-3f) && Near(shouldBeId.M30, 0.0f, 1e-3f));

    // A zero-scale matrix inverts to Identity rather than blowing up.
    CHECK(Near(FMatrix().Inverse().M00, 1.0f));

    // Scaling extraction: build a scaled rotation matrix and pull the scale back out.
    FMatrix scaled = FRotationMatrix(FRotator(0.0f, 90.0f, 0.0f));
    scaled.SetAxis(0, scaled.GetScaledAxis(EAxis::X) * 2.0f);
    scaled.SetAxis(1, scaled.GetScaledAxis(EAxis::Y) * 3.0f);
    scaled.SetAxis(2, scaled.GetScaledAxis(EAxis::Z) * 4.0f);
    CHECK(NearV(scaled.GetScaleVector(), FVector(2, 3, 4), 1e-3f));
    CHECK(Near(scaled.GetMaximumAxisScale(), 4.0f, 1e-3f));

    FMatrix extractFrom = scaled;
    CHECK(NearV(extractFrom.ExtractScaling(), FVector(2, 3, 4), 1e-3f));
    CHECK(Near(extractFrom.GetScaledAxis(EAxis::X).Size(), 1.0f, 1e-3f)); // scale removed in place

    FMatrix removeFrom = scaled;
    removeFrom.RemoveScaling();
    CHECK(Near(removeFrom.GetScaledAxis(EAxis::Y).Size(), 1.0f, kNormEps));

    // Point vs vector transforms: only positions pick up the translation.
    const FMatrix translate(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        10, 20, 30, 1);
    CHECK(NearV(static_cast<FVector>(translate.TransformPosition(FVector(1, 2, 3))), FVector(11, 22, 33)));
    CHECK(NearV(static_cast<FVector>(translate.TransformVector(FVector(1, 2, 3))), FVector(1, 2, 3)));
    CHECK(NearV(translate.InverseTransformPosition(FVector(11, 22, 33)), FVector(1, 2, 3), 1e-3f));

    // Frustum plane extraction reports failure on a degenerate plane.
    FPlane plane;
    CHECK(!FMatrix().GetFrustumNearPlane(plane));
    CHECK(translate.GetFrustumLeftPlane(plane));
}

static void TestRotationMatrices()
{
    const FRotator r(20.0f, -35.0f, 60.0f);
    const FVector v(0.4f, -1.3f, 2.0f);

    // FRotationMatrix rotates exactly like the rotator it was built from.
    const FRotationMatrix rm(r);
    CHECK(NearV(static_cast<FVector>(rm.TransformVector(v)), r.RotateVector(v), 1e-3f));
    CHECK(NearV(rm.GetOrigin(), FVector::ZeroVector)); // no translation

    // ... and its rows are orthonormal.
    CHECK(Near(rm.GetScaledAxis(EAxis::X).Size(), 1.0f, 1e-3f));
    CHECK(Near(rm.GetScaledAxis(EAxis::X) | rm.GetScaledAxis(EAxis::Y), 0.0f, 1e-3f));

    // FRotationTranslationMatrix adds the origin.
    const FRotationTranslationMatrix rtm(r, FVector(1, 2, 3));
    CHECK(NearV(rtm.GetOrigin(), FVector(1, 2, 3)));

    // FQuatRotationMatrix agrees with the quaternion it was built from.
    const FQuat q = r.Quaternion();
    const FQuatRotationMatrix qrm(q);
    CHECK(NearV(static_cast<FVector>(qrm.TransformVector(v)), q.RotateVector(v), 1e-3f));

    const FQuatRotationTranslationMatrix qrtm(q, FVector(4, 5, 6));
    CHECK(NearV(qrtm.GetOrigin(), FVector(4, 5, 6)));

    // Matrix -> quaternion and matrix -> rotator both recover the original orientation.
    CHECK(NearQ(qrm.ToQuat(), q, 1e-3f));
    CHECK(NearQ(FQuat(static_cast<const FMatrix&>(rm)), q, 1e-3f));
    CHECK(r.GetNormalized().Equals(rm.Rotator(), 0.05f));

    // A degenerate (zero) matrix converts to the identity quaternion rather than NaN.
    CHECK(FQuat(FMatrix()).IsIdentity());
}

static void TestTransform()
{
    CHECK(FTransform::Identity.Rotation == FQuat::Identity);
    CHECK(NearV(FTransform::Identity.Translation, FVector::ZeroVector));
    CHECK(NearV(FTransform::Identity.Scale3D, FVector::OneVector));
    CHECK(FTransform().IsRotationNormalized());
    CHECK(NearV(FTransform::Identity.TransformPosition(FVector(1, 2, 3)), FVector(1, 2, 3)));

    const FQuat qz(FVector(0.0f, 0.0f, 1.0f), kPi / 2.0f);
    const FTransform t(qz, FVector(1, 2, 3), FVector(2, 2, 2));

    // Position picks up scale, rotation and translation; a vector skips the translation.
    CHECK(NearV(t.TransformPosition(FVector(1, 0, 0)), FVector(1, 4, 3), 1e-3f));
    CHECK(NearV(t.TransformVector(FVector(1, 0, 0)), FVector(0, 2, 0), 1e-3f));
    CHECK(NearV(t.TransformPositionNoScale(FVector(1, 0, 0)), FVector(1, 3, 3), 1e-3f));
    CHECK(NearV(t.TransformVectorNoScale(FVector(1, 0, 0)), FVector(0, 1, 0), 1e-3f));
    CHECK(Near(t.GetDeterminant(), 8.0f));
    CHECK(Near(t.GetMaximumAxisScale(), 2.0f));
    CHECK(Near(t.GetMinimumAxisScale(), 2.0f));

    // Inverse and the InverseTransform* helpers both undo the transform.
    const FVector p(0.7f, -2.1f, 4.0f);
    CHECK(NearV(t.Inverse().TransformPosition(t.TransformPosition(p)), p, 1e-2f));
    CHECK(NearV(t.InverseTransformPosition(t.TransformPosition(p)), p, 1e-2f));
    CHECK(NearV(t.InverseTransformPositionNoScale(t.TransformPositionNoScale(p)), p, 1e-2f));

    // Composition: (a * b) applies a first, then b.
    const FTransform a(FQuat(FVector(0, 1, 0), kPi / 3.0f), FVector(1, 0, 0), FVector(2, 2, 2));
    const FTransform b(qz, FVector(0, 5, 0), FVector(3, 3, 3));
    const FTransform ab = a * b;
    CHECK(NearV(ab.TransformPosition(p), b.TransformPosition(a.TransformPosition(p)), 1e-2f));

    // GetRelativeTransform(other) undoes other: (a.GetRelativeTransform(b)) * b == a.
    const FTransform rel = a.GetRelativeTransform(b);
    CHECK(NearV((rel * b).TransformPosition(p), a.TransformPosition(p), 1e-2f));

    // Matrix round-trip: transform -> matrix -> transform.
    FTransform fromMatrix;
    fromMatrix.SetFromMatrix(t.ToMatrixWithScale());
    CHECK(NearV(fromMatrix.Translation, t.Translation, 1e-3f));
    CHECK(NearV(fromMatrix.Scale3D, t.Scale3D, 1e-2f));
    CHECK(NearQ(fromMatrix.Rotation, t.Rotation, 1e-2f));
    CHECK(NearRelV(fromMatrix.TransformPosition(p), t.TransformPosition(p), 5e-3f));

    // The FVector-basis ctor goes through the same matrix path.
    const FTransform basis(FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FVector(9, 9, 9));
    CHECK(NearV(basis.Translation, FVector(9, 9, 9)));
    // W lands on 0.9983, not 1.0: SetFromMatrix ends with Rotation.Normalize(), whose InvSqrt is the Quake III
    // approximation (identical in the C# source). The engine tolerates this — IsNormalized() allows 0.01.
    CHECK(NearQ(basis.Rotation, FQuat::Identity, kNormEps));
    CHECK(basis.Rotation.IsNormalized());

    // Rotator()/TransformRotation delegate to the quaternion.
    CHECK(t.Rotator().Equals(qz.Rotator(), 0.05f));
    CHECK(NearQ(t.TransformRotation(FQuat::Identity), qz));
    CHECK(NearQ(t.InverseTransformRotation(qz), FQuat::Identity, 1e-3f));

    // Mutators.
    FTransform mut = FTransform::Identity;
    mut.SetLocation(FVector(1, 1, 1));
    mut.SetScale3D(FVector(4, 4, 4));
    mut.SetRotation(qz);
    CHECK(NearV(mut.Translation, FVector(1, 1, 1)) && NearV(mut.Scale3D, FVector(4, 4, 4)));
    mut.ScaleTranslation(2.0f);
    CHECK(NearV(mut.Translation, FVector(2, 2, 2)));
    mut.RemoveScaling();
    CHECK(NearV(mut.Scale3D, FVector::OneVector));
    CHECK(NearV(mut.GetScaled(3.0f).Scale3D, FVector(3, 3, 3)));

    FTransform copyTo = FTransform::Identity;
    copyTo.CopyTranslation(t);
    copyTo.CopyRotation(t);
    copyTo.CopyScale3D(t);
    CHECK(copyTo.Equals(t, 1e-3f));
    CHECK(NearV(FTransform::SubstractTranslations(t, FTransform::Identity), t.Translation));

    // Unit axes of a scaled transform are the rotated basis vectors.
    CHECK(NearV(t.GetUnitAxis(EAxis::X), FVector(0, 1, 0), 1e-3f));
    CHECK(NearV(t.GetScaledAxis(EAxis::X), FVector(0, 2, 0), 1e-3f));

    CHECK(!t.ContainsNaN());
    CHECK(FTransform::AnyHasNegativeScale(FVector(1, -1, 1), FVector::OneVector));
    CHECK(!FTransform::AnyHasNegativeScale(FVector::OneVector, FVector::OneVector));

    // A negative scale routes through the matrix path instead of the quaternion path.
    const FTransform negative(qz, FVector(1, 2, 3), FVector(-2, 2, 2));
    const FTransform negComposed = negative * b;
    // The negative-scale path compounds two InvSqrt normalizations (RemoveScaling + Rotation.Normalize).
    CHECK(NearRelV(negComposed.TransformPosition(p), b.TransformPosition(negative.TransformPosition(p)), 5e-3f));

    // Multiplying with an unnormalized rotation is rejected.
    bool threw = false;
    try
    {
        const FTransform bad(FQuat(0.0f, 0.0f, 0.0f, 5.0f), FVector::ZeroVector, FVector::OneVector);
        (void) (bad * b);
    }
    catch (const std::invalid_argument&) { threw = true; }
    CHECK(threw);

    // GetSafeScaleReciprocal hardcodes 0 rather than producing an infinity.
    const FVector recip = FTransform::GetSafeScaleReciprocal(FVector(4.0f, 0.0f, -2.0f));
    CHECK(Near(recip.X, 0.25f) && recip.Y == 0.0f && Near(recip.Z, -0.5f));
}

static void TestPlane()
{
    const FPlane p(1.0f, 0.0f, 0.0f, 5.0f);
    CHECK(p.GetX() == 1.0f && p.GetY() == 0.0f && p.GetZ() == 0.0f && p.W == 5.0f);

    // PlaneDot is the signed distance along the normal.
    CHECK(Near(p.PlaneDot(FVector(5.0f, 0.0f, 0.0f)), 0.0f));  // on the plane
    CHECK(Near(p.PlaneDot(FVector(7.0f, 0.0f, 0.0f)), 2.0f));  // in front
    CHECK(Near(p.PlaneDot(FVector(1.0f, 0.0f, 0.0f)), -4.0f)); // behind

    // The (base, normal) ctor stores W as base | normal.
    const FPlane fromBase(FVector(0.0f, 3.0f, 0.0f), FVector(0.0f, 1.0f, 0.0f));
    CHECK(Near(fromBase.W, 3.0f));

    FPlane setter;
    setter.SetX(1.0f); setter.SetY(2.0f); setter.SetZ(3.0f);
    CHECK(NearV(setter.Vector, FVector(1, 2, 3)));

    CHECK(p == FPlane(1.0f, 0.0f, 0.0f, 5.0f));
    CHECK(p != FPlane(1.0f, 0.0f, 0.0f, 6.0f));
}

static void TestLayout()
{
    // FQuat and FPlane are read straight off the archive with Ar.Read<T>, so their layout must be blittable
    // and match the C# [StructLayout(Sequential)] sizes.
    static_assert(std::is_trivially_copyable_v<FQuat> && std::is_standard_layout_v<FQuat>, "FQuat must be blittable");
    static_assert(sizeof(FQuat) == 16, "FQuat must be 4 floats");
    static_assert(std::is_trivially_copyable_v<FPlane> && std::is_standard_layout_v<FPlane>, "FPlane must be blittable");
    static_assert(sizeof(FPlane) == 16, "FPlane must be 16 bytes (FVector + W)");
    static_assert(std::is_trivially_copyable_v<FRotator> && std::is_standard_layout_v<FRotator>, "FRotator must be blittable");
    static_assert(sizeof(FRotator) == 12, "FRotator must be 3 floats");
    static_assert(sizeof(FMatrix) == 64, "FMatrix must be 16 floats");
    static_assert(sizeof(FMatrix3x4) == 48, "FMatrix3x4 must be 12 floats");
    static_assert(sizeof(FTransform) == 40, "FTransform is a quat + two vectors");
    CHECK(true);
}

int main()
{
    TestRotator();
    TestQuat();
    TestRotatorQuatRoundTrip();
    TestMatrix();
    TestRotationMatrices();
    TestTransform();
    TestPlane();
    TestLayout();

    if (g_failures == 0)
    {
        std::printf("All geometry tests passed.\n");
        return 0;
    }
    std::printf("%d check(s) failed.\n", g_failures);
    return 1;
}
