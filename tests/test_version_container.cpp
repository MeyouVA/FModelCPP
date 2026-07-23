// Tests VersionContainer's two per-game lookup tables and the three readers that branch on them.
//
// The table half checks the boundary game for every option (the version the flag flips at, plus each
// per-game exception carved out of it), that a missing key throws rather than defaulting to false, and that
// the override maps are re-applied every time Game/Platform is reassigned.
//
// The consumer half proves the tables actually reach the readers: ByteProperty's MAP-mode width, the
// FScriptStruct Vector_NetQuantize arms, and UScriptMap's MapStructTypes key/value type substitution.
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "UE4/Assets/IPackage.h"
#include "UE4/Assets/Objects/FPropertyTagData.h"
#include "UE4/Assets/Objects/FScriptStruct.h"
#include "UE4/Assets/Objects/FStructFallback.h"
#include "UE4/Assets/Objects/UScriptMap.h"
#include "UE4/Assets/Objects/Properties/ByteProperty.h"
#include "UE4/Assets/Objects/Properties/IntProperty.h"
#include "UE4/Assets/Objects/Properties/StructProperty.h"
#include "UE4/Assets/Readers/FAssetArchive.h"
#include "UE4/Objects/Core/Math/FVector.h"
#include "UE4/Objects/Core/Misc/FGuid.h"
#include "UE4/Objects/UObject/FNameEntrySerialized.h"
#include "UE4/Readers/FByteArchive.h"
#include "UE4/Versions/ObjectVersion.h"
#include "UE4/Versions/VersionContainer.h"

using namespace CUE4Parse::UE4::Assets;
using namespace CUE4Parse::UE4::Assets::Objects;
using namespace CUE4Parse::UE4::Readers;
using namespace CUE4Parse::UE4::Versions;
using CUE4Parse::UE4::Assets::Objects::Properties::ByteProperty;
using CUE4Parse::UE4::Assets::Objects::Properties::IntProperty;
using CUE4Parse::UE4::Assets::Objects::Properties::ReadType;
using CUE4Parse::UE4::Assets::Objects::Properties::StructProperty;
using CUE4Parse::UE4::Assets::Readers::FAssetArchive;
using CUE4Parse::UE4::Objects::Core::Math::FVector;
using CUE4Parse::UE4::Objects::Core::Misc::FGuid;
using CUE4Parse::UE4::Objects::UObject::FNameEntrySerialized;

static int g_failures = 0;
#define CHECK(cond)                                                           \
    do {                                                                      \
        if (!(cond)) {                                                        \
            std::cerr << "FAIL: " << #cond << " (line " << __LINE__ << ")\n"; \
            ++g_failures;                                                     \
        }                                                                     \
    } while (0)

template <typename T>
static void AppendLE(std::vector<uint8_t>& buf, T value)
{
    uint8_t tmp[sizeof(T)];
    std::memcpy(tmp, &value, sizeof(T));
    buf.insert(buf.end(), tmp, tmp + sizeof(T));
}

// An FName pair (name index, extra index) as the asset archive reads it.
static void AppendFName(std::vector<uint8_t>& buf, int32_t index, int32_t extra = 0)
{
    AppendLE<int32_t>(buf, index);
    AppendLE<int32_t>(buf, extra);
}

class TestPackage : public IPackage
{
public:
    std::vector<FNameEntrySerialized> Names{FNameEntrySerialized("None")};
    std::string PkgName = "TestPackage";
    const std::string& GetName() const override { return PkgName; }
    const std::vector<FNameEntrySerialized>& NameMap() const override { return Names; }
    bool HasFlags(CUE4Parse::UE4::Objects::UObject::EPackageFlags) const override { return false; }
    ResolvedObject* ResolvePackageIndex(const CUE4Parse::UE4::Objects::UObject::FPackageIndex*) override
    { return nullptr; }
};

// --------------------------------------------------------------------------------------------------
// Options table
// --------------------------------------------------------------------------------------------------

static void TestOptionDefaults()
{
    const VersionContainer vc; // GAME_UE4_LATEST
    CHECK(vc["MorphTarget"] == true);
    CHECK(vc["StripAdditiveRefPose"] == false);
    CHECK(vc["SkeletalMesh.KeepMobileMinLODSettingOnDesktop"] == false);
    CHECK(vc["StaticMesh.KeepMobileMinLODSettingOnDesktop"] == false);
    CHECK(vc["ByteProperty.TMap64Bit"] == false);
    CHECK(vc["ByteProperty.TMap16Bit"] == false);
    CHECK(vc["ByteProperty.TMap8Bit"] == false);
    // The table is fully populated even where nothing reads the entry yet.
    CHECK(vc.Options.size() == 20);
}

static void TestMissingOptionThrows()
{
    const VersionContainer vc;
    bool threw = false;
    try { (void) vc["NoSuchOption"]; }
    catch (const std::out_of_range&) { threw = true; }
    CHECK(threw); // C#'s Dictionary indexer throws KeyNotFoundException rather than defaulting to false
}

// Each option's flip point, checked on both sides of the boundary.
static void TestOptionVersionBoundaries()
{
    CHECK(VersionContainer(GAME_UE4_27)["Vector_NetQuantize_AsStruct"] == false);
    CHECK(VersionContainer(GAME_UE5_0)["Vector_NetQuantize_AsStruct"] == true);

    CHECK(VersionContainer(GAME_UE4_24)["RawIndexBuffer.HasShouldExpandTo32Bit"] == false);
    CHECK(VersionContainer(GAME_UE4_25)["RawIndexBuffer.HasShouldExpandTo32Bit"] == true);

    CHECK(VersionContainer(GAME_UE4_27)["ShaderMap.UseNewCookedFormat"] == false);
    CHECK(VersionContainer(GAME_UE5_0)["ShaderMap.UseNewCookedFormat"] == true);

    CHECK(VersionContainer(GAME_UE4_23)["SkeletalMesh.UseNewCookedFormat"] == false);
    CHECK(VersionContainer(GAME_UE4_24)["SkeletalMesh.UseNewCookedFormat"] == true);

    CHECK(VersionContainer(GAME_UE4_26)["SkeletalMesh.HasRayTracingData"] == false);
    CHECK(VersionContainer(GAME_UE4_27)["SkeletalMesh.HasRayTracingData"] == true);

    CHECK(VersionContainer(GAME_UE4_24)["StaticMesh.HasRayTracingGeometry"] == false);
    CHECK(VersionContainer(GAME_UE4_25)["StaticMesh.HasRayTracingGeometry"] == true);

    CHECK(VersionContainer(GAME_UE4_25)["StaticMesh.HasVisibleInRayTracing"] == false);
    CHECK(VersionContainer(GAME_UE4_26)["StaticMesh.HasVisibleInRayTracing"] == true);

    CHECK(VersionContainer(GAME_UE4_22)["StaticMesh.UseNewCookedFormat"] == false);
    CHECK(VersionContainer(GAME_UE4_23)["StaticMesh.UseNewCookedFormat"] == true);

    CHECK(VersionContainer(GAME_UE4_22)["VirtualTextures"] == false);
    CHECK(VersionContainer(GAME_UE4_23)["VirtualTextures"] == true);

    CHECK(VersionContainer(GAME_UE4_24)["SoundWave.UseAudioStreaming"] == false);
    CHECK(VersionContainer(GAME_UE4_25)["SoundWave.UseAudioStreaming"] == true);

    CHECK(VersionContainer(GAME_UE4_16)["AnimSequence.HasCompressedRawSize"] == false);
    CHECK(VersionContainer(GAME_UE4_17)["AnimSequence.HasCompressedRawSize"] == true);

    // Exists in every engine version except UE4.15 exactly.
    CHECK(VersionContainer(GAME_UE4_14)["StaticMesh.HasLODsShareStaticLighting"] == true);
    CHECK(VersionContainer(GAME_UE4_15)["StaticMesh.HasLODsShareStaticLighting"] == false);
    CHECK(VersionContainer(GAME_UE4_16)["StaticMesh.HasLODsShareStaticLighting"] == true);
}

// The per-game carve-outs, each checked against a sibling game of the same engine version that keeps the
// default — otherwise a wrong engine-version boundary would pass the exception check by accident.
static void TestPerGameExceptions()
{
    CHECK(VersionContainer(GAME_DeltaForce)["RawIndexBuffer.HasShouldExpandTo32Bit"] == false);
    CHECK(VersionContainer(GAME_ArenaBreakoutMobile)["RawIndexBuffer.HasShouldExpandTo32Bit"] == false);

    // UE4_25_Plus sorts below UE4_27 but still has ray tracing data.
    CHECK(VersionContainer(GAME_UE4_25_Plus)["SkeletalMesh.HasRayTracingData"] == true);

    // Back4Blood is a 4.25-era game that nonetheless writes bVisibleInRayTracing.
    CHECK(VersionContainer(GAME_Back4Blood)["StaticMesh.HasVisibleInRayTracing"] == true);

    // 4.25+ games that ship sound waves without audio streaming.
    CHECK(VersionContainer(GAME_UE4_28)["SoundWave.UseAudioStreaming"] == false);
    CHECK(VersionContainer(GAME_GTATheTrilogyDefinitiveEdition)["SoundWave.UseAudioStreaming"] == false);
    CHECK(VersionContainer(GAME_ReadyOrNot)["SoundWave.UseAudioStreaming"] == false);
    CHECK(VersionContainer(GAME_BladeAndSoul)["SoundWave.UseAudioStreaming"] == false);
    CHECK(VersionContainer(GAME_Stray)["SoundWave.UseAudioStreaming"] == false);
    CHECK(VersionContainer(GAME_UE4_27)["SoundWave.UseAudioStreaming"] == true);
}

// HasNavCollision is the one option that reads Ver rather than Game, so it must survive the ctor's
// SetGame -> SetVer ordering.
static void TestNavCollisionFollowsVer()
{
    CHECK(VersionContainer(GAME_UE4_LATEST)["StaticMesh.HasNavCollision"] == true);
    CHECK(VersionContainer(GAME_GearsOfWar4)["StaticMesh.HasNavCollision"] == false);
    CHECK(VersionContainer(GAME_TEKKEN7)["StaticMesh.HasNavCollision"] == false);

    // A version pinned below STATIC_MESH_STORE_NAV_COLLISION turns it off regardless of the game.
    const VersionContainer old(
        GAME_UE4_LATEST, ETexturePlatform::DesktopMobile,
        FPackageFileVersion::CreateUE4Version(
            static_cast<int32_t>(EUnrealEngineObjectUE4Version::STATIC_MESH_STORE_NAV_COLLISION) - 1));
    CHECK(old.bExplicitVer);
    CHECK(old["StaticMesh.HasNavCollision"] == false);
}

static void TestOptionOverrides()
{
    VersionContainer vc(GAME_UE4_27, ETexturePlatform::DesktopMobile, {},
                        {{"Vector_NetQuantize_AsStruct", true}, {"StripAdditiveRefPose", true}});
    CHECK(vc["Vector_NetQuantize_AsStruct"] == true); // overrides the computed false
    CHECK(vc["StripAdditiveRefPose"] == true);
    CHECK(vc["MorphTarget"] == true); // untouched entries keep their computed value

    // Reassigning Game rebuilds the table from scratch — the overrides must be re-applied on top.
    vc.SetGame(GAME_UE4_20);
    CHECK(vc["Vector_NetQuantize_AsStruct"] == true);
    CHECK(vc["StaticMesh.UseNewCookedFormat"] == false); // recomputed for the new game
    vc.SetPlatform(ETexturePlatform::Playstation5);
    CHECK(vc["StripAdditiveRefPose"] == true);

    // The indexer setter writes through without going near the override map, so the next Init drops it.
    vc.SetOption("MorphTarget", false);
    CHECK(vc["MorphTarget"] == false);
    vc.SetGame(GAME_UE4_20);
    CHECK(vc["MorphTarget"] == true);
}

// --------------------------------------------------------------------------------------------------
// MapStructTypes table
// --------------------------------------------------------------------------------------------------

static void TestMapStructTypes()
{
    const VersionContainer vc;
    CHECK(vc.MapStructTypes.size() == 6);
    CHECK(vc.MapStructTypes.at("BindingIdToReferences") == VersionContainer::FMapStructTypes("Guid", ""));
    CHECK(vc.MapStructTypes.at("UserParameterRedirects") ==
          VersionContainer::FMapStructTypes("NiagaraVariable", "NiagaraVariable"));
    CHECK(vc.MapStructTypes.at("Tracks") == VersionContainer::FMapStructTypes("MovieSceneTrackIdentifier", ""));
    CHECK(vc.MapStructTypes.at("SubSequences") == VersionContainer::FMapStructTypes("MovieSceneSequenceID", ""));
    CHECK(vc.MapStructTypes.at("Hierarchy") == VersionContainer::FMapStructTypes("MovieSceneSequenceID", ""));

    // The one entry that moves: the value type lost its plural in 4.19.
    CHECK(VersionContainer(GAME_UE4_18).MapStructTypes.at("TrackSignatureToTrackIdentifier").second ==
          "MovieSceneTrackIdentifiers");
    CHECK(VersionContainer(GAME_UE4_19).MapStructTypes.at("TrackSignatureToTrackIdentifier").second ==
          "MovieSceneTrackIdentifier");

    VersionContainer overridden(GAME_UE4_LATEST, ETexturePlatform::DesktopMobile, {}, {},
                                {{"Tracks", {"Guid", "Vector"}}, {"MyMap", {"Guid", ""}}});
    CHECK(overridden.MapStructTypes.at("Tracks") == VersionContainer::FMapStructTypes("Guid", "Vector"));
    CHECK(overridden.MapStructTypes.at("MyMap").first == "Guid");
    CHECK(overridden.MapStructTypes.size() == 7);
    overridden.SetGame(GAME_UE4_18);
    CHECK(overridden.MapStructTypes.at("Tracks") == VersionContainer::FMapStructTypes("Guid", "Vector"));
}

// --------------------------------------------------------------------------------------------------
// Consumers
// --------------------------------------------------------------------------------------------------

// ByteProperty widens its MAP-mode read to 8/16/64 bits when the matching option is set.
static void TestBytePropertyMapWidth()
{
    struct Case { const char* option; size_t expectedBytes; };
    const Case cases[] = {
        {nullptr, 1},                  // no option set: a single byte
        {"ByteProperty.TMap8Bit", 1},
        {"ByteProperty.TMap16Bit", 2},
        {"ByteProperty.TMap64Bit", 8},
    };

    for (const auto& c : cases)
    {
        std::map<std::string, bool> overrides;
        if (c.option != nullptr) overrides[c.option] = true;
        const VersionContainer versions(GAME_UE4_LATEST, ETexturePlatform::DesktopMobile, {}, overrides);

        // 0x5A in the low byte, then filler the wider reads must swallow.
        std::vector<uint8_t> bytes{0x5A, 0, 0, 0, 0, 0, 0, 0};
        FByteArchive base("byte", bytes, versions);
        TestPackage pkg;
        FAssetArchive ar(base, &pkg);
        const ByteProperty value(ar, ReadType::MAP);
        CHECK(value.Value == 0x5A);
        CHECK(static_cast<size_t>(ar.Position) == c.expectedBytes);
    }

    // NORMAL mode ignores the option entirely.
    const VersionContainer wide(GAME_UE4_LATEST, ETexturePlatform::DesktopMobile, {},
                                {{"ByteProperty.TMap64Bit", true}});
    std::vector<uint8_t> bytes{0x7B, 0, 0, 0, 0, 0, 0, 0};
    FByteArchive base("byte", bytes, wide);
    TestPackage pkg;
    FAssetArchive ar(base, &pkg);
    const ByteProperty value(ar, ReadType::NORMAL);
    CHECK(value.Value == 0x7B);
    CHECK(ar.Position == 1);
}

// Vector_NetQuantize* reads a bare FVector before UE5 and a tagged property bag from UE5 on.
static void TestVectorNetQuantizeArm()
{
    const char* names[] = {"Vector_NetQuantize", "Vector_NetQuantize10", "Vector_NetQuantize100",
                           "Vector_NetQuantizeNormal"};

    for (const char* name : names)
    {
        // UE4: three floats, straight into an FVector.
        {
            const VersionContainer versions(
                GAME_UE4_LATEST, ETexturePlatform::DesktopMobile,
                FPackageFileVersion(864, static_cast<int32_t>(EUnrealEngineObjectUE4Version::AUTOMATIC_VERSION), 0));
            CHECK(versions["Vector_NetQuantize_AsStruct"] == false);

            std::vector<uint8_t> bytes;
            AppendLE<float>(bytes, 1.5f);
            AppendLE<float>(bytes, 2.5f);
            AppendLE<float>(bytes, 3.5f);
            FByteArchive base("vnq", bytes, versions);
            TestPackage pkg;
            FAssetArchive ar(base, &pkg);
            const FScriptStruct value(ar, name, nullptr, ReadType::NORMAL);
            const auto* v = value.Get<FVector>();
            CHECK(v != nullptr);
            if (v != nullptr) CHECK(v->X == 1.5f && v->Y == 2.5f && v->Z == 3.5f);
            CHECK(ar.Position == 12);
        }

        // UE5: a property bag, so only the terminating "None" name is consumed for an empty one.
        {
            const VersionContainer versions(GAME_UE5_0);
            CHECK(versions["Vector_NetQuantize_AsStruct"] == true);

            std::vector<uint8_t> bytes;
            AppendFName(bytes, 0); // "None"
            FByteArchive base("vnq", bytes, versions);
            TestPackage pkg;
            FAssetArchive ar(base, &pkg);
            const FScriptStruct value(ar, name, nullptr, ReadType::NORMAL);
            CHECK(value.Get<FVector>() == nullptr);
            CHECK(value.AsFallback() != nullptr);
            CHECK(ar.Position == 8);
        }

        // ZERO still short-circuits to a default FVector in both cases.
        {
            const VersionContainer versions(GAME_UE5_0);
            std::vector<uint8_t> bytes(16, 0xAB);
            FByteArchive base("vnq", bytes, versions);
            TestPackage pkg;
            FAssetArchive ar(base, &pkg);
            const FScriptStruct value(ar, name, nullptr, ReadType::ZERO);
            CHECK(value.Get<FVector>() != nullptr);
            CHECK(ar.Position == 0);
        }
    }
}

// UScriptMap substitutes the key/value struct type named by MapStructTypes when the tag itself carries none.
static void TestUScriptMapStructTypes()
{
    const FGuid guid(0x11111111, 0x22222222, 0x33333333, 0x44444444);

    // numKeysToRemove, numEntries, then one (FGuid key, int32 value) pair.
    std::vector<uint8_t> bytes;
    AppendLE<int32_t>(bytes, 0);
    AppendLE<int32_t>(bytes, 1);
    AppendLE<uint32_t>(bytes, guid.A);
    AppendLE<uint32_t>(bytes, guid.B);
    AppendLE<uint32_t>(bytes, guid.C);
    AppendLE<uint32_t>(bytes, guid.D);
    AppendLE<int32_t>(bytes, 4242);

    FPropertyTagData tagData;
    tagData.Name = "BindingIdToReferences"; // a built-in entry: key struct "Guid", value left alone
    tagData.Type = "MapProperty";
    tagData.InnerType = "StructProperty";
    tagData.ValueType = "IntProperty";

    const VersionContainer versions;
    FByteArchive base("map", bytes, versions);
    TestPackage pkg;
    FAssetArchive ar(base, &pkg);
    const UScriptMap map(ar, &tagData, ReadType::NORMAL);

    CHECK(map.Properties.size() == 1);
    if (map.Properties.size() == 1)
    {
        const auto* key = dynamic_cast<const StructProperty*>(map.Properties[0].first.get());
        CHECK(key != nullptr);
        // Without the table this would have no struct type and fall back to a tagged property bag.
        if (key != nullptr)
        {
            const auto* g = key->Value.Get<FGuid>();
            CHECK(g != nullptr);
            if (g != nullptr) CHECK(*g == guid);
        }
        const auto* value = dynamic_cast<const IntProperty*>(map.Properties[0].second.get());
        CHECK(value != nullptr);
        if (value != nullptr) CHECK(value->Value == 4242);
    }
    CHECK(ar.Position == 28);
}

// A property name that is not in the table reads with the tag's own (absent) descriptor, so the struct key
// falls back to a tagged property bag — the control for the test above.
static void TestUScriptMapWithoutTableEntry()
{
    std::vector<uint8_t> bytes;
    AppendLE<int32_t>(bytes, 0);
    AppendLE<int32_t>(bytes, 1);
    AppendFName(bytes, 0); // empty property bag for the key
    AppendLE<int32_t>(bytes, 7);

    FPropertyTagData tagData;
    tagData.Name = "SomeUnknownMap";
    tagData.Type = "MapProperty";
    tagData.InnerType = "StructProperty";
    tagData.ValueType = "IntProperty";

    const VersionContainer versions;
    FByteArchive base("map", bytes, versions);
    TestPackage pkg;
    FAssetArchive ar(base, &pkg);
    const UScriptMap map(ar, &tagData, ReadType::NORMAL);

    CHECK(map.Properties.size() == 1);
    if (map.Properties.size() == 1)
    {
        const auto* key = dynamic_cast<const StructProperty*>(map.Properties[0].first.get());
        CHECK(key != nullptr);
        if (key != nullptr)
        {
            CHECK(key->Value.Get<FGuid>() == nullptr);
            CHECK(key->Value.AsFallback() != nullptr);
        }
    }
    CHECK(ar.Position == 20);
}

int main()
{
    try
    {
        TestOptionDefaults();
        TestMissingOptionThrows();
        TestOptionVersionBoundaries();
        TestPerGameExceptions();
        TestNavCollisionFollowsVer();
        TestOptionOverrides();
        TestMapStructTypes();
        TestBytePropertyMapWidth();
        TestVectorNetQuantizeArm();
        TestUScriptMapStructTypes();
        TestUScriptMapWithoutTableEntry();
    }
    catch (const std::exception& e)
    {
        std::cerr << "EXCEPTION: " << e.what() << "\n";
        return 1;
    }

    if (g_failures != 0)
    {
        std::cerr << g_failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "test_version_container OK\n";
    return 0;
}
