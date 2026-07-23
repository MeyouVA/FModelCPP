// Tests the FXxx{Object,Custom}Version family and the VersionUtils::CustomVer lookup they all sit on.
//
// Every one of the 40 headers is included here, which is itself the broadest check the family gets: the
// generated headers only ever compile because something includes them, and including all of them at once
// also proves the per-file namespaces keep their (unscoped, colliding-by-name) enum members apart.
//
// The behavioural half covers CustomVer's three answers in precedence order (provider override table, then
// the owning package summary, then -1 = "guess from the game"), and then the game-guess ladders themselves:
// a plain relational ladder, the equality arms that single out one game, the multi-game `or` arm, and the
// arms that return a raw (Type)(-1) for "older than the plugin existed".
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "UE4/Assets/IPackage.h"
#include "UE4/Assets/Readers/FAssetArchive.h"
#include "UE4/Objects/Core/Misc/FGuid.h"
#include "UE4/Objects/Core/Serialization/FCustomVersionContainer.h"
#include "UE4/Objects/UObject/FNameEntrySerialized.h"
#include "UE4/Objects/UObject/FPackageFileSummary.h"
#include "UE4/Readers/FByteArchive.h"

#include "UE4/Versions/FAnimObjectVersion.h"
#include "UE4/Versions/FAnimPhysObjectVersion.h"
#include "UE4/Versions/FAssetRegistryVersion.h"
#include "UE4/Versions/FBlueprintsObjectVersion.h"
#include "UE4/Versions/FControlRigObjectVersion.h"
#include "UE4/Versions/FCoreObjectVersion.h"
#include "UE4/Versions/FCurveExpressionCustomVersion.h"
#include "UE4/Versions/FDNAAssetCustomVersion.h"
#include "UE4/Versions/FDestructionObjectVersion.h"
#include "UE4/Versions/FEditorObjectVersion.h"
#include "UE4/Versions/FExternalPhysicsCustomObjectVersion.h"
#include "UE4/Versions/FFoliageCustomVersion.h"
#include "UE4/Versions/FFortniteMainBranchObjectVersion.h"
#include "UE4/Versions/FFortniteReleaseBranchCustomObjectVersion.h"
#include "UE4/Versions/FFortniteSeasonBranchObjectVersion.h"
#include "UE4/Versions/FFrameworkObjectVersion.h"
#include "UE4/Versions/FHeightmapTextureEdgeSnapshotCustomVersion.h"
#include "UE4/Versions/FInstancedStructCustomVersion.h"
#include "UE4/Versions/FInterchangeCustomVersion.h"
#include "UE4/Versions/FLiveLinkCustomVersion.h"
#include "UE4/Versions/FMobileObjectVersion.h"
#include "UE4/Versions/FNiagaraCustomVersion.h"
#include "UE4/Versions/FNiagaraObjectVersion.h"
#include "UE4/Versions/FOverlappingVerticesCustomVersion.h"
#include "UE4/Versions/FOverridablePropertyBagCustomVersion.h"
#include "UE4/Versions/FPCGCustomVersion.h"
#include "UE4/Versions/FPhysicsObjectVersion.h"
#include "UE4/Versions/FPropertyBagCustomVersion.h"
#include "UE4/Versions/FRecomputeTangentCustomVersion.h"
#include "UE4/Versions/FReflectionCaptureObjectVersion.h"
#include "UE4/Versions/FReleaseObjectVersion.h"
#include "UE4/Versions/FRenderingObjectVersion.h"
#include "UE4/Versions/FRigVMObjectVersion.h"
#include "UE4/Versions/FSequencerObjectVersion.h"
#include "UE4/Versions/FSkeletalMeshCustomVersion.h"
#include "UE4/Versions/FStateTreeInstanceStorageCustomVersion.h"
#include "UE4/Versions/FUE5MainStreamObjectVersion.h"
#include "UE4/Versions/FUE5ReleaseStreamObjectVersion.h"
#include "UE4/Versions/FUE5SpecialProjectStreamObjectVersion.h"
#include "UE4/Versions/FVariantManagerObjectVersion.h"
#include "UE4/Versions/VersionUtils.h"

using namespace CUE4Parse::UE4::Assets;
using namespace CUE4Parse::UE4::Readers;
using namespace CUE4Parse::UE4::Versions;
using CUE4Parse::UE4::Assets::Readers::FAssetArchive;
using CUE4Parse::UE4::Objects::Core::Misc::FGuid;
using CUE4Parse::UE4::Objects::Core::Serialization::FCustomVersion;
using CUE4Parse::UE4::Objects::Core::Serialization::FCustomVersionContainer;
using CUE4Parse::UE4::Objects::UObject::FNameEntrySerialized;
using CUE4Parse::UE4::Objects::UObject::FPackageFileSummary;

static int g_failures = 0;
#define CHECK(cond)                                                           \
    do {                                                                      \
        if (!(cond)) {                                                        \
            std::cerr << "FAIL: " << #cond << " (line " << __LINE__ << ")\n"; \
            ++g_failures;                                                     \
        }                                                                     \
    } while (0)

// A package whose summary the test can dictate, so CustomVer's second lookup can be driven directly.
class TestPackage : public IPackage
{
public:
    FPackageFileSummary Summary;
    std::vector<FNameEntrySerialized> Names{FNameEntrySerialized("None")};
    std::string PkgName = "TestPackage";

    const std::string& GetName() const override { return PkgName; }
    const std::vector<FNameEntrySerialized>& NameMap() const override { return Names; }
    bool HasFlags(CUE4Parse::UE4::Objects::UObject::EPackageFlags) const override { return false; }
    const FPackageFileSummary* GetSummary() const override { return &Summary; }
    ResolvedObject* ResolvePackageIndex(const CUE4Parse::UE4::Objects::UObject::FPackageIndex*) override
    { return nullptr; }
};

// --------------------------------------------------------------------------------------------------
// CustomVer lookup order
// --------------------------------------------------------------------------------------------------

static void TestCustomVerWithoutAnySource()
{
    // A bare (non-asset) archive with no override table has no custom versions at all.
    std::vector<uint8_t> empty;
    FByteArchive ar("test", empty, VersionContainer(GAME_UE4_27));
    CHECK(CustomVer(ar, FCoreObjectVersion::GUID) == -1);
    // ...so Get() falls through to the game ladder.
    CHECK(FCoreObjectVersion::Get(ar) == FCoreObjectVersion::FProperties);
}

static void TestCustomVerFromOverrideTable()
{
    std::vector<uint8_t> empty;
    VersionContainer versions(GAME_UE4_27);
    versions.CustomVersions = std::make_shared<FCustomVersionContainer>(
        std::vector<FCustomVersion>{FCustomVersion(FCoreObjectVersion::GUID, 2)});
    FByteArchive ar("test", empty, versions);

    CHECK(CustomVer(ar, FCoreObjectVersion::GUID) == 2);
    // The override beats what the game alone would have said (FProperties == 4).
    CHECK(FCoreObjectVersion::Get(ar) == FCoreObjectVersion::EnumProperties);

    // A key that is not in the table still falls through to the game ladder.
    CHECK(CustomVer(ar, FAnimObjectVersion::GUID) == -1);
}

static void TestCustomVerFromPackageSummary()
{
    std::vector<uint8_t> empty;
    FByteArchive base("test", empty, VersionContainer(GAME_UE4_27));
    TestPackage pkg;
    pkg.Summary.bUnversioned = false;
    pkg.Summary.CustomVersionContainer =
        FCustomVersionContainer(std::vector<FCustomVersion>{FCustomVersion(FCoreObjectVersion::GUID, 1)});
    FAssetArchive ar(base, &pkg);

    CHECK(CustomVer(ar, FCoreObjectVersion::GUID) == 1);
    CHECK(FCoreObjectVersion::Get(ar) == FCoreObjectVersion::MaterialInputNativeSerialize);

    // An unversioned package's table is not consulted at all, so the game ladder wins again.
    pkg.Summary.bUnversioned = true;
    CHECK(CustomVer(ar, FCoreObjectVersion::GUID) == -1);
    CHECK(FCoreObjectVersion::Get(ar) == FCoreObjectVersion::FProperties);
}

static void TestOverrideBeatsPackageSummary()
{
    std::vector<uint8_t> empty;
    VersionContainer versions(GAME_UE4_27);
    versions.CustomVersions = std::make_shared<FCustomVersionContainer>(
        std::vector<FCustomVersion>{FCustomVersion(FCoreObjectVersion::GUID, 3)});
    FByteArchive base("test", empty, versions);

    TestPackage pkg;
    pkg.Summary.bUnversioned = false;
    pkg.Summary.CustomVersionContainer =
        FCustomVersionContainer(std::vector<FCustomVersion>{FCustomVersion(FCoreObjectVersion::GUID, 1)});
    FAssetArchive ar(base, &pkg);

    // Both sources answer; the provider override is the one that counts.
    CHECK(CustomVer(ar, FCoreObjectVersion::GUID) == 3);
}

// --------------------------------------------------------------------------------------------------
// The game-guess ladders
// --------------------------------------------------------------------------------------------------

static FCoreObjectVersion::Type CoreFor(EGame game)
{
    std::vector<uint8_t> empty;
    FByteArchive ar("test", empty, VersionContainer(game));
    return FCoreObjectVersion::Get(ar);
}

static void TestRelationalLadder()
{
    // Both sides of every boundary in FCoreObjectVersion's ladder.
    CHECK(CoreFor(GAME_UE4_11) == FCoreObjectVersion::BeforeCustomVersionWasAdded);
    CHECK(CoreFor(GAME_UE4_12) == FCoreObjectVersion::MaterialInputNativeSerialize);
    CHECK(CoreFor(GAME_UE4_14) == FCoreObjectVersion::MaterialInputNativeSerialize);
    CHECK(CoreFor(GAME_UE4_15) == FCoreObjectVersion::EnumProperties);
    CHECK(CoreFor(GAME_UE4_21) == FCoreObjectVersion::EnumProperties);
    CHECK(CoreFor(GAME_UE4_22) == FCoreObjectVersion::SkeletalMaterialEditorDataStripping);
    CHECK(CoreFor(GAME_UE4_24) == FCoreObjectVersion::SkeletalMaterialEditorDataStripping);
    CHECK(CoreFor(GAME_UE4_25) == FCoreObjectVersion::FProperties);
    CHECK(CoreFor(GAME_UE5_5) == FCoreObjectVersion::FProperties);
}

static void TestEqualityArmWinsOverRelational()
{
    // GAME_TEKKEN7 is a UE4.14-era game, so the relational ladder alone would answer with the 4.14 rung.
    // FEditorObjectVersion carves it out with an equality arm listed *first*, and order is what makes that
    // work -- a ladder that sorted the relational arms first would never reach it.
    std::vector<uint8_t> empty;
    FByteArchive tekken("test", empty, VersionContainer(GAME_TEKKEN7));
    FByteArchive ue414("test", empty, VersionContainer(GAME_UE4_14));
    CHECK(FEditorObjectVersion::Get(tekken) != FEditorObjectVersion::Get(ue414));
    CHECK(FEditorObjectVersion::Get(tekken) == FEditorObjectVersion::ComboBoxControllerSupportUpdate);
    CHECK(FEditorObjectVersion::Get(ue414) == FEditorObjectVersion::RefactorMeshEditorMaterials);
}

static void TestMultiGameOrArm()
{
    // FReleaseObjectVersion's one `A or B or C` arm -- all three games must land on the same rung, and it
    // must not be the one their engine version alone would give.
    std::vector<uint8_t> empty;
    for (const EGame game : {GAME_DarkPicturesAnthologyManofMedan,
                             GAME_DarkPicturesAnthologyLittleHope,
                             GAME_DarkPicturesAnthologyTheDevilinMe})
    {
        FByteArchive ar("test", empty, VersionContainer(game));
        CHECK(FReleaseObjectVersion::Get(ar) ==
              FReleaseObjectVersion::GeometryCollectionCacheRemovesMassToLocal);
    }
}

static void TestNegativeSentinelArm()
{
    // FDNAAssetCustomVersion returns a raw (Type)(-1) for games older than the plugin, which is *below*
    // BeforeCustomVersionWasAdded == 0 and so distinguishable from it.
    std::vector<uint8_t> empty;
    FByteArchive old("test", empty, VersionContainer(GAME_UE4_25));
    FByteArchive now("test", empty, VersionContainer(GAME_UE4_26));
    CHECK(static_cast<int>(FDNAAssetCustomVersion::Get(old)) == -1);
    CHECK(FDNAAssetCustomVersion::Get(now) == FDNAAssetCustomVersion::LatestVersion);
    CHECK(static_cast<int>(FDNAAssetCustomVersion::LatestVersion) == 0);
}

static void TestVersionsOutsideTheVersionsNamespace()
{
    // FExternalPhysicsCustomObjectVersion is the one member of the family whose C# namespace is
    // Objects.UObject rather than Versions; the port keeps that, so it is spelled differently here.
    std::vector<uint8_t> empty;
    FByteArchive ar("test", empty, VersionContainer(GAME_UE4_25));
    using namespace CUE4Parse::UE4::Objects::UObject;
    CHECK(FExternalPhysicsCustomObjectVersion::Get(ar) ==
          FExternalPhysicsCustomObjectVersion::PhysicsMaterialSleepCounterThreshold);
}

static void TestGuidsAreDistinct()
{
    // A copy-paste in a GUID literal would silently make two version keys alias each other.
    const std::vector<FGuid> guids{
        FAnimObjectVersion::GUID, FAnimPhysObjectVersion::GUID, FBlueprintsObjectVersion::GUID,
        FControlRigObjectVersion::GUID, FCoreObjectVersion::GUID, FCurveExpressionCustomVersion::GUID,
        FDNAAssetCustomVersion::GUID, FDestructionObjectVersion::GUID, FEditorObjectVersion::GUID,
        FFoliageCustomVersion::GUID, FFortniteMainBranchObjectVersion::GUID,
        FFortniteReleaseBranchCustomObjectVersion::GUID, FFortniteSeasonBranchObjectVersion::GUID,
        FFrameworkObjectVersion::GUID, FHeightmapTextureEdgeSnapshotCustomVersion::GUID,
        FInstancedStructCustomVersion::GUID, FInterchangeCustomVersion::GUID, FLiveLinkCustomVersion::GUID,
        FMobileObjectVersion::GUID, FNiagaraCustomVersion::GUID, FNiagaraObjectVersion::GUID,
        FOverlappingVerticesCustomVersion::GUID, FOverridablePropertyBagCustomVersion::GUID,
        FPCGCustomVersion::GUID, FPhysicsObjectVersion::GUID, FPropertyBagCustomVersion::GUID,
        FRecomputeTangentCustomVersion::GUID, FReflectionCaptureObjectVersion::GUID,
        FReleaseObjectVersion::GUID, FRenderingObjectVersion::GUID, FRigVMObjectVersion::GUID,
        FSequencerObjectVersion::GUID, FSkeletalMeshCustomVersion::GUID,
        FStateTreeInstanceStorageCustomVersion::GUID, FUE5MainStreamObjectVersion::GUID,
        FUE5ReleaseStreamObjectVersion::GUID, FUE5SpecialProjectStreamObjectVersion::GUID,
        FVariantManagerObjectVersion::GUID,
        CUE4Parse::UE4::Objects::UObject::FExternalPhysicsCustomObjectVersion::GUID};

    for (size_t i = 0; i < guids.size(); ++i)
    {
        CHECK(guids[i].IsValid());
        for (size_t j = i + 1; j < guids.size(); ++j)
            CHECK(!(guids[i] == guids[j]));
    }
}

static void TestAssetRegistryVersion()
{
    // TrySerializeVersion only trusts the trailing version int when the leading GUID matches.
    const FGuid registryGuid(0x717F9EE7, 0xE9B0493A, 0x88B39132, 0x1B388107);

    std::vector<uint8_t> good;
    const auto guidBytes = registryGuid.AsByteSpan();
    good.insert(good.end(), guidBytes.begin(), guidBytes.end());
    for (const uint8_t b : {uint8_t(3), uint8_t(0), uint8_t(0), uint8_t(0)}) good.push_back(b);

    FByteArchive ar("test", good, VersionContainer(GAME_UE5_3));
    FAssetRegistryVersionType version = FAssetRegistryVersionType::LatestVersion;
    FAssetRegistryVersion::TrySerializeVersion(ar, version);
    CHECK(version == FAssetRegistryVersionType::ChangedAssetData);
    CHECK(ar.Position == 20);

    // A foreign GUID means the stream predates versioning; the version int is left unread.
    std::vector<uint8_t> bad(20, 0);
    bad[0] = 0x01;
    FByteArchive ar2("test", bad, VersionContainer(GAME_UE5_3));
    FAssetRegistryVersionType version2 = FAssetRegistryVersionType::LatestVersion;
    FAssetRegistryVersion::TrySerializeVersion(ar2, version2);
    CHECK(version2 == FAssetRegistryVersionType::PreVersioning);
    CHECK(ar2.Position == 16);
}

int main()
{
    TestCustomVerWithoutAnySource();
    TestCustomVerFromOverrideTable();
    TestCustomVerFromPackageSummary();
    TestOverrideBeatsPackageSummary();
    TestRelationalLadder();
    TestEqualityArmWinsOverRelational();
    TestMultiGameOrArm();
    TestNegativeSentinelArm();
    TestVersionsOutsideTheVersionsNamespace();
    TestGuidsAreDistinct();
    TestAssetRegistryVersion();

    if (g_failures == 0) std::cout << "test_custom_versions: all checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
