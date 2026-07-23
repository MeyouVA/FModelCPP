// Tests FScriptStruct's named-struct table: for each ported entry, feed the archive the exact bytes the
// struct's reader consumes and check the boxed value comes back with the right type and contents. Also
// covers the ZERO arm (no bytes read, default-constructed value), the FStructFallback default arm, and
// ToString's "{value} ({TypeName})" shape.
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "UE4/Assets/IPackage.h"
#include "UE4/Assets/Objects/FScriptStruct.h"
#include "UE4/Assets/Objects/Properties/FPropertyTagType.h"
#include "UE4/Assets/Readers/FAssetArchive.h"
#include "UE4/Objects/Core/Math/FBox.h"
#include "UE4/Objects/Core/Math/FColor.h"
#include "UE4/Objects/Core/Math/FIntPoint.h"
#include "UE4/Objects/Core/Math/FLinearColor.h"
#include "UE4/Objects/Core/Math/FPlane.h"
#include "UE4/Objects/Core/Math/FQuat.h"
#include "UE4/Objects/Core/Math/FRotator.h"
#include "UE4/Objects/Core/Math/FSphere.h"
#include "UE4/Objects/Core/Math/FTwoVectors.h"
#include "UE4/Objects/Core/Math/FVector.h"
#include "UE4/Objects/Core/Math/FVector2D.h"
#include "UE4/Objects/Core/Math/FVector4.h"
#include "UE4/Objects/Core/Math/TIntVector.h"
#include "UE4/Objects/Core/Misc/FDateTime.h"
#include "UE4/Objects/Core/Misc/FGuid.h"
#include "UE4/Objects/Engine/Curves/RichCurve.h"
#include "UE4/Objects/UObject/FNameEntrySerialized.h"
#include "UE4/Objects/UObject/FSoftObjectPath.h"
#include "UE4/Readers/FByteArchive.h"
#include "UE4/Versions/ObjectVersion.h"
#include "UE4/Versions/VersionContainer.h"

using namespace CUE4Parse::UE4::Assets;
using namespace CUE4Parse::UE4::Assets::Objects;
using namespace CUE4Parse::UE4::Objects::Core::Math;
using namespace CUE4Parse::UE4::Readers;
using namespace CUE4Parse::UE4::Versions;
using CUE4Parse::UE4::Assets::Objects::Properties::ReadType;
using CUE4Parse::UE4::Assets::Readers::FAssetArchive;
using CUE4Parse::UE4::Objects::Core::Misc::FDateTime;
using CUE4Parse::UE4::Objects::Core::Misc::FGuid;
using CUE4Parse::UE4::Objects::Engine::Curves::ERichCurveInterpMode;
using CUE4Parse::UE4::Objects::Engine::Curves::FRichCurveKey;
using CUE4Parse::UE4::Objects::UObject::FNameEntrySerialized;
using CUE4Parse::UE4::Objects::UObject::FSoftObjectPath;

static int g_failures = 0;
#define CHECK(cond)                                                           \
    do {                                                                      \
        if (!(cond)) {                                                        \
            std::cerr << "FAIL: " << #cond << " (line " << __LINE__ << ")\n"; \
            ++g_failures;                                                     \
        }                                                                     \
    } while (0)

static bool Near(float a, float b) { return std::fabs(a - b) < 1e-4f; }

template <typename T>
static void AppendLE(std::vector<uint8_t>& buf, T value)
{
    uint8_t tmp[sizeof(T)];
    std::memcpy(tmp, &value, sizeof(T));
    buf.insert(buf.end(), tmp, tmp + sizeof(T));
}

static void AppendFString(std::vector<uint8_t>& buf, const std::string& s)
{
    AppendLE<int32_t>(buf, static_cast<int32_t>(s.size() + 1));
    buf.insert(buf.end(), s.begin(), s.end());
    buf.push_back(0);
}

class TestPackage : public IPackage
{
public:
    std::vector<FNameEntrySerialized> Names;
    std::string PkgName = "TestPackage";
    const std::string& GetName() const override { return PkgName; }
    const std::vector<FNameEntrySerialized>& NameMap() const override { return Names; }
    bool HasFlags(CUE4Parse::UE4::Objects::UObject::EPackageFlags) const override { return false; }
    ResolvedObject* ResolvePackageIndex(const CUE4Parse::UE4::Objects::UObject::FPackageIndex*) override
    { return nullptr; }
};

// UE4 (FVector = floats, ReadFReal reads 4 bytes) and UE5 (LARGE_WORLD_COORDINATES: doubles) containers.
static const VersionContainer& Ue4Versions()
{
    static const VersionContainer vc(GAME_UE4_LATEST, ETexturePlatform::DesktopMobile,
                                     FPackageFileVersion(864, static_cast<int32_t>(EUnrealEngineObjectUE4Version::AUTOMATIC_VERSION), 0));
    return vc;
}

// Drives one named-struct entry: builds the archive over `bytes`, constructs the FScriptStruct and hands
// it back along with how many bytes were consumed.
struct Harness
{
    TestPackage Pkg;
    std::vector<uint8_t> Bytes;

    FScriptStruct Read(const std::string& structName, ReadType type = ReadType::NORMAL,
                       int64_t* outConsumed = nullptr)
    {
        FByteArchive base("struct", Bytes, Ue4Versions());
        FAssetArchive ar(base, &Pkg);
        FScriptStruct value(ar, structName, nullptr, type);
        if (outConsumed != nullptr) *outConsumed = ar.Position;
        return value;
    }
};

// --------------------------------------------------------------------------------------------------

static void TestVectorFamily()
{
    {
        Harness h;
        AppendLE<float>(h.Bytes, 1.0f);
        AppendLE<float>(h.Bytes, 2.0f);
        AppendLE<float>(h.Bytes, 3.0f);
        int64_t consumed = 0;
        const auto value = h.Read("Vector", ReadType::NORMAL, &consumed);
        const auto* v = value.Get<FVector>();
        CHECK(v != nullptr && Near(v->X, 1.0f) && Near(v->Y, 2.0f) && Near(v->Z, 3.0f));
        CHECK(consumed == 12);
        // The wrong type must not alias the right one.
        CHECK(value.Get<FVector4>() == nullptr);
        CHECK(value.AsFallback() == nullptr);
    }
    {
        Harness h;
        AppendLE<float>(h.Bytes, 4.0f);
        AppendLE<float>(h.Bytes, 5.0f);
        const auto value = h.Read("Vector2D");
        const auto* v = value.Get<FVector2D>();
        CHECK(v != nullptr && Near(v->X, 4.0f) && Near(v->Y, 5.0f));
    }
    {
        Harness h;
        for (float f : {1.0f, 2.0f, 3.0f, 4.0f}) AppendLE<float>(h.Bytes, f);
        const auto value = h.Read("Vector4");
        const auto* v = value.Get<FVector4>();
        CHECK(v != nullptr && Near(v->W, 4.0f));
    }
    {
        // Vector3d reads three doubles into TIntVector3<double> — a different boxed type from Vector3f.
        Harness h;
        for (double d : {1.5, 2.5, 3.5}) AppendLE<double>(h.Bytes, d);
        int64_t consumed = 0;
        const auto value = h.Read("Vector3d", ReadType::NORMAL, &consumed);
        const auto* v = value.Get<TIntVector3<double>>();
        CHECK(v != nullptr && v->X == 1.5 && v->Z == 3.5);
        CHECK(consumed == 24);
        CHECK(value.Get<TIntVector3<float>>() == nullptr);
    }
    {
        // "VectorDouble" is an alias of the same arm.
        Harness h;
        for (double d : {9.0, 8.0, 7.0}) AppendLE<double>(h.Bytes, d);
        const auto value = h.Read("VectorDouble");
        const auto* v = value.Get<TIntVector3<double>>();
        CHECK(v != nullptr && v->Y == 8.0);
    }
    {
        // Vector_NetQuantize takes the FVector arm (the Versions option is unported — see FScriptStruct.h).
        Harness h;
        for (float f : {1.0f, 1.0f, 1.0f}) AppendLE<float>(h.Bytes, f);
        const auto value = h.Read("Vector_NetQuantize100");
        CHECK(value.Get<FVector>() != nullptr);
    }
}

static void TestIntegerVectorFamily()
{
    {
        Harness h;
        AppendLE<int32_t>(h.Bytes, 7);
        AppendLE<int32_t>(h.Bytes, -3);
        const auto value = h.Read("IntPoint");
        const auto* p = value.Get<FIntPoint>();
        CHECK(p != nullptr && p->X == 7 && p->Y == -3);
    }
    {
        // The signed and unsigned 2-component arms box distinct types even though the bytes are the same.
        Harness h;
        AppendLE<uint32_t>(h.Bytes, 4000000000u);
        AppendLE<uint32_t>(h.Bytes, 1u);
        const auto value = h.Read("UintVector2");
        const auto* v = value.Get<TIntVector2<uint32_t>>();
        CHECK(v != nullptr && v->X == 4000000000u);
        CHECK(value.Get<TIntVector2<int32_t>>() == nullptr);
    }
    {
        Harness h;
        AppendLE<int64_t>(h.Bytes, -5);
        AppendLE<int64_t>(h.Bytes, 6);
        AppendLE<int64_t>(h.Bytes, 7);
        AppendLE<int64_t>(h.Bytes, 8);
        int64_t consumed = 0;
        const auto value = h.Read("Int64Vector4", ReadType::NORMAL, &consumed);
        const auto* v = value.Get<TIntVector4<int64_t>>();
        CHECK(v != nullptr && v->X == -5 && v->W == 8);
        CHECK(consumed == 32);
    }
}

static void TestMiscStructs()
{
    {
        Harness h;
        h.Bytes = {0x10, 0x20, 0x30, 0x40}; // B, G, R, A
        const auto value = h.Read("Color");
        const auto* c = value.Get<FColor>();
        CHECK(c != nullptr && c->B == 0x10 && c->G == 0x20 && c->R == 0x30 && c->A == 0x40);
    }
    {
        Harness h;
        for (float f : {1.0f, 0.5f, 0.25f, 1.0f}) AppendLE<float>(h.Bytes, f);
        const auto value = h.Read("LinearColor");
        const auto* c = value.Get<FLinearColor>();
        CHECK(c != nullptr && Near(c->G, 0.5f));
    }
    {
        Harness h;
        for (uint32_t part : {1u, 2u, 3u, 4u}) AppendLE<uint32_t>(h.Bytes, part);
        const auto value = h.Read("Guid");
        const auto* g = value.Get<FGuid>();
        CHECK(g != nullptr && g->A == 1 && g->D == 4);
    }
    {
        // DateTime and Timespan share the FDateTime arm.
        Harness h;
        AppendLE<int64_t>(h.Bytes, 637000000000000000ll);
        CHECK(h.Read("DateTime").Get<FDateTime>() != nullptr);
        CHECK(h.Read("Timespan").Get<FDateTime>() != nullptr);
    }
    {
        // A struct read through FAssetArchive rather than the raw archive: FName + FString.
        Harness h;
        h.Pkg.Names = {FNameEntrySerialized("None"), FNameEntrySerialized("/Game/Path/Asset.Asset")};
        AppendLE<int32_t>(h.Bytes, 1); // FName index
        AppendLE<int32_t>(h.Bytes, 0); // FName extra index
        AppendFString(h.Bytes, "SubPath");
        const auto value = h.Read("SoftObjectPath");
        const auto* p = value.Get<FSoftObjectPath>();
        CHECK(p != nullptr && p->SubPathString == "SubPath");
        // ...and its three aliases resolve to the same boxed type.
        CHECK(h.Read("SoftClassPath").Get<FSoftObjectPath>() != nullptr);
        CHECK(h.Read("StringAssetReference").Get<FSoftObjectPath>() != nullptr);
        CHECK(h.Read("StringClassReference").Get<FSoftObjectPath>() != nullptr);
    }
    {
        Harness h;
        for (float f : {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f}) AppendLE<float>(h.Bytes, f);
        const auto value = h.Read("TwoVectors");
        const auto* tv = value.Get<FTwoVectors>();
        CHECK(tv != nullptr && Near(tv->V1.X, 1.0f) && Near(tv->V2.Z, 6.0f));
    }
    {
        // RichCurveKey is raw: 3 mode bytes + 1 pad byte + 6 floats = 28 bytes.
        Harness h;
        h.Bytes = {static_cast<uint8_t>(ERichCurveInterpMode::RCIM_Cubic), 0, 0, 0};
        AppendLE<float>(h.Bytes, 2.0f);  // Time
        AppendLE<float>(h.Bytes, 20.0f); // Value
        for (int i = 0; i < 4; i++) AppendLE<float>(h.Bytes, 0.0f);
        int64_t consumed = 0;
        const auto value = h.Read("RichCurveKey", ReadType::NORMAL, &consumed);
        const auto* key = value.Get<FRichCurveKey>();
        CHECK(key != nullptr && Near(key->Time, 2.0f) && Near(key->Value, 20.0f));
        CHECK(key != nullptr && key->InterpMode == ERichCurveInterpMode::RCIM_Cubic);
        CHECK(consumed == 28);
    }
}

// The arms that read component-wise and therefore must sequence their reads explicitly.
static void TestSequencedArms()
{
    {
        Harness h;
        for (float f : {1.0f, 2.0f, 3.0f}) AppendLE<float>(h.Bytes, f); // vector
        AppendLE<float>(h.Bytes, 4.0f);                                 // W — read after, never before
        int64_t consumed = 0;
        const auto value = h.Read("Plane4f", ReadType::NORMAL, &consumed);
        const auto* p = value.Get<FPlane>();
        CHECK(p != nullptr && Near(p->Vector.X, 1.0f) && Near(p->Vector.Z, 3.0f) && Near(p->W, 4.0f));
        CHECK(consumed == 16);
    }
    {
        Harness h;
        for (float f : {10.0f, 20.0f, 30.0f}) AppendLE<float>(h.Bytes, f);
        const auto value = h.Read("Rotator3f");
        const auto* r = value.Get<FRotator>();
        CHECK(r != nullptr && Near(r->Pitch, 10.0f) && Near(r->Yaw, 20.0f) && Near(r->Roll, 30.0f));
    }
    {
        Harness h;
        for (float f : {1.0f, 2.0f, 3.0f, 9.0f}) AppendLE<float>(h.Bytes, f);
        const auto value = h.Read("Sphere3f");
        const auto* s = value.Get<FSphere>();
        CHECK(s != nullptr && Near(s->Center.Y, 2.0f) && Near(s->W, 9.0f));
    }
    {
        Harness h;
        for (float f : {0.0f, 0.0f, 0.0f, 1.0f}) AppendLE<float>(h.Bytes, f);
        const auto value = h.Read("Quat4f");
        const auto* q = value.Get<FQuat>();
        CHECK(q != nullptr && Near(q->W, 1.0f));
    }
    {
        // The 4d arms narrow to float (this port's FQuat holds floats) but still consume 8 bytes each.
        Harness h;
        for (double d : {0.0, 0.0, 0.0, 1.0}) AppendLE<double>(h.Bytes, d);
        int64_t consumed = 0;
        const auto value = h.Read("Quat4d", ReadType::NORMAL, &consumed);
        const auto* q = value.Get<FQuat>();
        CHECK(q != nullptr && Near(q->W, 1.0f));
        CHECK(consumed == 32);
    }
}

static void TestZeroArmReadsNothing()
{
    // ReadType::ZERO means "the unversioned header said this value is all zeroes": default-construct,
    // consume no bytes. Every named entry must honour it.
    for (const char* name : {"Vector", "Vector4", "Guid", "Color", "IntPoint", "Plane4f", "Quat4d",
                             "Rotator3f", "Sphere3f", "RichCurveKey", "SoftObjectPath", "Matrix"})
    {
        Harness h;
        h.Bytes.assign(64, 0xAB); // garbage the arm must not touch
        int64_t consumed = -1;
        const auto value = h.Read(name, ReadType::ZERO, &consumed);
        CHECK(value.StructType != nullptr);
        if (consumed != 0) std::cerr << "  (ZERO arm consumed bytes for " << name << ")\n";
        CHECK(consumed == 0);
    }

    Harness h;
    const auto zeroVector = h.Read("Vector", ReadType::ZERO);
    const auto* v = zeroVector.Get<FVector>();
    CHECK(v != nullptr && v->X == 0.0f && v->Y == 0.0f && v->Z == 0.0f);
}

static void TestFallbackArmAndToString()
{
    // An unnamed struct falls back to the tagged property bag (a lone "None" terminator here).
    {
        Harness h;
        AppendLE<int32_t>(h.Bytes, 0); // FName "None"
        AppendLE<int32_t>(h.Bytes, 0);
        h.Pkg.Names = {FNameEntrySerialized("None")};
        const auto value = h.Read("SomeGameSpecificStruct");
        const auto* fallback = value.AsFallback();
        CHECK(fallback != nullptr && fallback->Properties.empty());
        CHECK(value.Get<FVector>() == nullptr);
        CHECK(value.ToString() == "[0 properties] (FStructFallback)");
    }
    // KeyHandleMap is the one named entry that deliberately reads nothing and boxes an empty fallback.
    {
        Harness h;
        h.Bytes.assign(16, 0xCD);
        int64_t consumed = -1;
        const auto value = h.Read("KeyHandleMap", ReadType::NORMAL, &consumed);
        CHECK(value.AsFallback() != nullptr && consumed == 0);
    }
    // ToString is "{value} ({TypeName})", with the type name stripped of its namespace.
    {
        Harness h;
        AppendLE<int32_t>(h.Bytes, 3);
        AppendLE<int32_t>(h.Bytes, 4);
        CHECK(h.Read("IntPoint").ToString() == "X: 3, Y: 4 (FIntPoint)");
    }
    {
        Harness h;
        for (double d : {1.0, 2.0, 3.0}) AppendLE<double>(h.Bytes, d);
        const auto text = h.Read("Vector3d").ToString();
        CHECK(text.find("(TIntVector3<double>)") != std::string::npos);
    }
    // A default-constructed FScriptStruct has no value at all.
    {
        const FScriptStruct empty;
        CHECK(empty.StructType == nullptr);
        CHECK(empty.ToString() == "(null)");
        CHECK(empty.Get<FVector>() == nullptr);
    }
    // C#'s FScriptStruct(IUStruct) ctor: box a value directly.
    {
        FScriptStruct made(FScriptStruct::Make<FIntPoint>(FIntPoint{1, 2}));
        const auto* p = made.Get<FIntPoint>();
        CHECK(p != nullptr && p->X == 1 && p->Y == 2);
    }
}

int main()
{
    try {
    TestVectorFamily();
    TestIntegerVectorFamily();
    TestMiscStructs();
    TestSequencedArms();
    TestZeroArmReadsNothing();
    TestFallbackArmAndToString();
    } catch (const std::exception& e) { std::cerr << "EXCEPTION: " << e.what() << "\n"; return 2; }

    if (g_failures == 0)
    {
        std::cout << "test_script_struct: all checks passed\n";
        return 0;
    }
    std::cerr << g_failures << " check(s) failed.\n";
    return 1;
}
