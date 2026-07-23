// Tests the second bulk enum pass: every remaining C# file in the tree that contains only enums, ported
// wherever it lived rather than as one directory family.
//
// As with test_wwise_fmod_enums, including all of them at once IS most of the test -- a generated header
// that nothing includes is never compiled, so this file is what makes the 60-odd headers below real. The
// previously-ported enum-only headers are included too, so the set stays a complete inventory rather than
// drifting into "the ones that happened to be new that day".
//
// The behavioural half pins what a mechanical translation can get wrong:
//   * the [Description] tables, which are runtime data (FModel looks them up for i18n), not decoration;
//   * a member value that C# writes as a ternary over *another* enum's member;
//   * the `LatestPlusOne - 1` idiom the version enums all use to name their newest member;
//   * a file whose declared namespace disagrees with its folder, kept as-is rather than tidied.
#include <cstdint>
#include <iostream>
#include <string>
#include <type_traits>

#include "Compression/CompressionMethod.h"
#include "MappingsProvider/Usmap/EUsmapCompressionMethod.h"
#include "MappingsProvider/Usmap/EUsmapVersion.h"
#include "UE4/AssetRegistry/Objects/ELoadOrder.h"
#include "UE4/Assets/Exports/Animation/AnimationCompressionFormat.h"
#include "UE4/Assets/Exports/Animation/AnimationKeyFormat.h"
#include "UE4/Assets/Exports/Animation/EAdditiveAnimationType.h"
#include "UE4/Assets/Exports/Animation/EAdditiveBasePoseType.h"
#include "UE4/Assets/Exports/Animation/EAnimInterpolationType.h"
#include "UE4/Assets/Exports/Animation/EBoneTranslationRetargetingMode.h"
#include "UE4/Assets/Exports/Component/TextRender/EHorizTextAligment.h"
#include "UE4/Assets/Exports/Component/TextRender/EVerticalTextAligment.h"
#include "UE4/Assets/Exports/EObjectFlags.h"
#include "UE4/Assets/Exports/Engine/ECurveTableMode.h"
#include "UE4/Assets/Exports/FastGeoStreaming/FastGeoEnums.h"
#include "UE4/Assets/Exports/Material/EBlendMode.h"
#include "UE4/Assets/Exports/Material/EMaterialFormat.h"
#include "UE4/Assets/Exports/Material/EMaterialShadingModel.h"
#include "UE4/Assets/Exports/Material/EMobileSpecularMask.h"
#include "UE4/Assets/Exports/Material/ETextureChannel.h"
#include "UE4/Assets/Exports/Material/ETranslucencyLightingMode.h"
#include "UE4/Assets/Exports/Nanite/ENaniteMeshFormat.h"
#include "UE4/Assets/Exports/Texture/ETextureCookPlatformTilingSettings.h"
#include "UE4/Assets/Exports/Texture/ETexturePlatform.h"
#include "UE4/Assets/Exports/Texture/TextureAddress.h"
#include "UE4/Assets/Exports/Texture/TextureCompressionSettings.h"
#include "UE4/Assets/Exports/Texture/TextureFilter.h"
#include "UE4/Assets/Exports/Texture/TextureGroup.h"
#include "UE4/Assets/Exports/WorldPartition/DataLayer/EDataLayerLoadFilter.h"
#include "UE4/Assets/Exports/WorldPartition/DataLayer/EDataLayerRuntimeState.h"
#include "UE4/Assets/Exports/WorldPartition/DataLayer/EDataLayerType.h"
#include "UE4/Assets/Exports/WorldPartition/ERuntimePartitionCellBoundsMethod.h"
#include "UE4/Assets/Exports/Wwise/EWwiseEventDestroyOptions.h"
#include "UE4/Assets/Exports/Wwise/EWwiseGroupType.h"
#include "UE4/Assets/Exports/Wwise/EWwiseLanguageRequirement.h"
#include "UE4/Assets/Exports/Wwise/EWwisePackagingStrategy.h"
#include "UE4/Assets/Exports/Wwise/EWwiseSoundBankType.h"
#include "UE4/Assets/Objects/EBulkDataFlags.h"
#include "UE4/Assets/Utils/PayloadType.h"
#include "UE4/Kismet/EExprToken.h"
#include "UE4/Lua/unluac/EUnluacErrorCode.h"
#include "UE4/Lua/unluac/EUnluacFlags.h"
#include "UE4/Objects/Chaos/GeometryCollection/EManagedArrayType.h"
#include "UE4/Objects/Core/Misc/ECompressionFlags.h"
#include "UE4/Objects/Core/RHI/RHIDefenitions.h"
#include "UE4/Objects/Core/i18N/ELocMetaVersion.h"
#include "UE4/Objects/Core/i18N/ELocResVersion.h"
#include "UE4/Objects/Engine/ELightUnits.h"
#include "UE4/Objects/Engine/EdGraph/EPinContainerType.h"
#include "UE4/Objects/RigVM/ERigVMMemoryType.h"
#include "UE4/Objects/RigVM/ERigVMOpCode.h"
#include "UE4/Objects/RigVM/ERigVMPinDirection.h"
#include "UE4/Objects/RigVM/ERigVMRegisterType.h"
#include "UE4/Objects/UObject/CoreNetTypes.h"
#include "UE4/Objects/UObject/EClassFlags.h"
#include "UE4/Objects/UObject/EFunctionFlags.h"
#include "UE4/Objects/UObject/EPackageFlags.h"
#include "UE4/Objects/UObject/EStructFlags.h"
#include "UE4/Versions/ELanguage.h"
#include "UE4/VirtualFileCache/EVFCFileVersion.h"
#include "UE4/VirtualFileCache/Manifest/EManifestMetaVersion.h"
#include "UE4/VirtualFileCache/Manifest/EManifestStorageFlags.h"

static int g_failures = 0;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            std::cerr << "FAIL: " << #cond << " (line " << __LINE__ << ")\n"; \
            ++g_failures;                                                  \
        }                                                                  \
    } while (0)

template <typename E>
static constexpr auto Raw(E value) { return static_cast<std::underlying_type_t<E>>(value); }

// ---------------------------------------------------------------- [Description] tables

// FModel calls GetDescription() on these at runtime -- Creator/Utils.cs uses the result as an
// internationalisation lookup key, so a dropped or mistyped string is a user-visible bug, not a cosmetic
// one. The strings must survive exactly, including the parenthetical.
namespace Mat = CUE4Parse::UE4::Assets::Exports::Material;
namespace Tex = CUE4Parse::UE4::Assets::Exports::Texture;
namespace TextRender = CUE4Parse::UE4::Assets::Exports::Component::TextRender;

static void TestDescriptions()
{
    CHECK(std::string(Mat::Description(Mat::EBlendMode::BLEND_Opaque)) == "Opaque");
    CHECK(std::string(Mat::Description(Mat::EBlendMode::BLEND_AlphaComposite)) ==
          "AlphaComposite (Premultiplied Alpha)");
    CHECK(std::string(Mat::Description(Mat::EBlendMode::BLEND_MAX)) == "MAX");

    // A member with no [Description] reports null rather than a guessed string; C#'s extension falls back
    // to the member name, which callers must do for themselves here.
    CHECK(Mat::Description(Mat::EBlendMode::BLEND_TranslucentColoredTransmittance) == nullptr);

    CHECK(std::string(Tex::Description(Tex::TextureAddress::TA_Wrap)) == "Wrap");
    CHECK(std::string(Tex::Description(Tex::TextureAddress::TA_Mirror)) == "Mirror");
    // TA_MAX is a count, not a mode, and carries no description.
    CHECK(Tex::Description(Tex::TextureAddress::TA_MAX) == nullptr);

    CHECK(std::string(TextRender::Description(TextRender::EVerticalTextAligment::EVRTA_TextTop)) ==
          "Text Top");
}

// ---------------------------------------------------------------- a ternary over another enum

// C# writes CF_DepthNearOrEqual as `ERHIZBuffer.IsInverted != 0 ? CF_GreaterEqual : CF_LessEqual`.
// CUE4Parse hardcodes IsInverted to 0 (upstream UE computes it as FarPlane < NearPlane), so the false
// branch always wins -- but the expression is kept rather than folded, so that if IsInverted is ever
// corrected the four depth comparisons follow automatically. These checks pin the current answer.
namespace RHI = CUE4Parse::UE4::Objects::Core::RHI;

static void TestRhiDepthComparisons()
{
    CHECK(Raw(RHI::ERHIZBuffer::IsInverted) == 0);
    CHECK(RHI::ECompareFunction::CF_DepthNearOrEqual == RHI::ECompareFunction::CF_LessEqual);
    CHECK(RHI::ECompareFunction::CF_DepthNear == RHI::ECompareFunction::CF_Less);
    CHECK(RHI::ECompareFunction::CF_DepthFartherOrEqual == RHI::ECompareFunction::CF_GreaterEqual);
    CHECK(RHI::ECompareFunction::CF_DepthFarther == RHI::ECompareFunction::CF_Greater);

    // The counts that follow the real members must not have been disturbed by the aliases at the end.
    CHECK(Raw(RHI::ECompareFunction::ECompareFunction_Num) == 8);
    CHECK(Raw(RHI::ECompareFunction::ECompareFunction_NumBits) == 3);
    // A large one from the same file, to prove the multi-enum split held.
    CHECK(Raw(RHI::EPrimitiveType::PT_NumBits) == 6);
}

// ---------------------------------------------------------------- the LatestPlusOne idiom

// Every version enum in the tree names its newest member `Latest = LatestPlusOne - 1`. Getting this wrong
// (off by one, or resolving LatestPlusOne before the members that precede it) would silently mis-gate every
// version check built on it.
namespace Usmap = CUE4Parse::MappingsProvider::Usmap;
namespace i18N = CUE4Parse::UE4::Objects::Core::i18N;

static void TestLatestPlusOne()
{
    CHECK(Usmap::EUsmapVersion::Latest == Usmap::EUsmapVersion::ExplicitEnumValues);
    CHECK(Raw(Usmap::EUsmapVersion::Initial) == 0);
    CHECK(Raw(Usmap::EUsmapVersion::Latest) == 4);
    CHECK(Raw(Usmap::EUsmapVersion::LatestPlusOne) == 5);

    CHECK(i18N::ELocResVersion::Latest == i18N::ELocResVersion::Optimized_CityHash64_UTF16);
    CHECK(i18N::ELocMetaVersion::Latest == i18N::ELocMetaVersion::AddedIsUGC);
    // Legacy is 0 and is what a file without the magic number reports.
    CHECK(Raw(i18N::ELocResVersion::Legacy) == 0);
}

// ---------------------------------------------------------------- namespace faithfulness

// EManagedArrayType.cs lives in UE4/Objects/Chaos/GeometryCollection but declares
// CUE4Parse.UE4.Chaos.GeometryCollection -- no Objects. That is a quirk of the C# source, and the port
// keeps it rather than tidying it, so a call site translated from C# still resolves. This compiles only if
// the namespace really is the odd one.
static void TestNamespaceQuirkIsPreserved()
{
    using CUE4Parse::UE4::Chaos::GeometryCollection::EManagedArrayType;
    CHECK(Raw(EManagedArrayType::FNoneType) == 0);
    CHECK(Raw(EManagedArrayType::FVectorType) == 1);
    // The 5.4+ additions sit at the end and must not have been reordered.
    CHECK(Raw(EManagedArrayType::FFImplicitObjectRefCountedPtrType) >
          Raw(EManagedArrayType::FTPBDRigidParticle3fUniquePtrType));
}

// ---------------------------------------------------------------- multi-enum files

// EExprToken.cs and FastGeoEnums.cs each hold several unrelated enums; the split has to keep every one of
// them, not just the first.
static void TestMultiEnumFiles()
{
    using namespace CUE4Parse::UE4::Kismet;
    CHECK(Raw(EExprToken::EX_LocalVariable) == 0x00);
    CHECK(Raw(EBlueprintTextLiteralType::Empty) == 0);

    using namespace CUE4Parse::UE4::Assets::Exports::FastGeoStreaming;
    CHECK(Raw(EComponentMobility::Static) == 0);
}

// ---------------------------------------------------------------- flags in this batch

namespace Manifest = CUE4Parse::UE4::VirtualFileCache::Manifest;

static void TestManifestStorageFlags()
{
    CHECK(Raw(Manifest::EManifestStorageFlags::None) == 0);
    CHECK(Raw(Manifest::EManifestStorageFlags::Compressed) == 1);
    CHECK(Raw(Manifest::EManifestStorageFlags::Encrypted) == 2);
}

// ---------------------------------------------------------------- underlying types

// These enums are read straight off the wire, so a widened underlying type would consume the wrong number
// of bytes.
static void TestUnderlyingTypes()
{
    static_assert(std::is_same_v<std::underlying_type_t<Usmap::EUsmapVersion>, uint8_t>);
    static_assert(std::is_same_v<std::underlying_type_t<i18N::ELocResVersion>, uint8_t>);
    static_assert(std::is_same_v<std::underlying_type_t<Mat::EBlendMode>, uint8_t>);
    static_assert(std::is_same_v<std::underlying_type_t<Tex::TextureAddress>, uint8_t>);
}

int main()
{
    TestDescriptions();
    TestRhiDepthComparisons();
    TestLatestPlusOne();
    TestNamespaceQuirkIsPreserved();
    TestMultiEnumFiles();
    TestManifestStorageFlags();
    TestUnderlyingTypes();

    if (g_failures == 0) std::cout << "test_bulk_enums: all checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
