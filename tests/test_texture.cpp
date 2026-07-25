// Tests the Texture export tree.
//
// Three layers, cheapest first:
//   1. Including every header is itself part of the test -- most of this tree is header-only, and a header
//      nothing includes never compiles. The include list doubles as an inventory of what was ported.
//   2. The pure logic that a mechanical translation gets wrong without a game to check against: the
//      PixelFormat block-geometry maths, the four PackedData bit masks, the odd upper-bound loop in
//      FVirtualTextureTileOffsetData::GetTileOffset, and the name -> EPixelFormat table that stands in for
//      C#'s Enum.TryParse (every member must round-trip, or a cooked texture silently reads PF_Unknown).
//   3. An end-to-end read of a hand-built cooked package: a real .uasset carrying one Texture2D export with
//      a two-mip DXT5 chain and inline bulk payloads, loaded through Package and walked back out. That is
//      the part that pins the wire format -- strip flags, the cooked flag, the per-format skip offset, the
//      platform-data header, and each mip's bulk-data header -- and it also proves the mip bytes can be read
//      back out of the package's export archive.
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

#include "UE4/Assets/Package.h"
#include "UE4/Assets/ResolvedObject.h"
#include "UE4/Assets/ObjectTypeRegistry.h"
#include "UE4/Objects/UObject/FPackageFileSummary.h"
#include "UE4/Readers/FByteArchive.h"
#include "UE4/Versions/VersionContainer.h"
#include "UE4/Versions/ObjectVersion.h"

// The tree under test.
#include "UE4/Assets/Exports/Texture/ETextureCookPlatformTilingSettings.h"
#include "UE4/Assets/Exports/Texture/ETexturePlatform.h"
#include "UE4/Assets/Exports/Texture/FTexture2DMipMap.h"
#include "UE4/Assets/Exports/Texture/FTexturePlatformData.h"
#include "UE4/Assets/Exports/Texture/FVirtualTextureBuiltData.h"
#include "UE4/Assets/Exports/Texture/FVirtualTextureDataChunk.h"
#include "UE4/Assets/Exports/Texture/PixelFormat.h"
#include "UE4/Assets/Exports/Texture/TextureAddress.h"
#include "UE4/Assets/Exports/Texture/TextureCompressionSettings.h"
#include "UE4/Assets/Exports/Texture/TextureFilter.h"
#include "UE4/Assets/Exports/Texture/TextureGroup.h"
#include "UE4/Assets/Exports/Texture/UCurveLinearColorAtlas.h"
#include "UE4/Assets/Exports/Texture/ULightMapTexture2D.h"
#include "UE4/Assets/Exports/Texture/ULightMapVirtualTexture2D.h"
#include "UE4/Assets/Exports/Texture/UMediaTexture.h"
#include "UE4/Assets/Exports/Texture/UPaperSprite.h"
#include "UE4/Assets/Exports/Texture/URuntimeVirtualTextureStreamingProxy.h"
#include "UE4/Assets/Exports/Texture/UShadowMapTexture2D.h"
#include "UE4/Assets/Exports/Texture/USubstanceAirTexture2D.h"
#include "UE4/Assets/Exports/Texture/UTerrainWeightMapTexture.h"
#include "UE4/Assets/Exports/Texture/UTexture.h"
#include "UE4/Assets/Exports/Texture/UTexture2D.h"
#include "UE4/Assets/Exports/Texture/UTexture2DArray.h"
#include "UE4/Assets/Exports/Texture/UTextureAllMipDataProviderFactory.h"
#include "UE4/Assets/Exports/Texture/UTextureCube.h"
#include "UE4/Assets/Exports/Texture/UTextureFlipBook.h"
#include "UE4/Assets/Exports/Texture/UTextureLightProfile.h"
#include "UE4/Assets/Exports/Texture/UTextureMipDataProviderFactory.h"
#include "UE4/Assets/Exports/Texture/UTextureMovie.h"
#include "UE4/Assets/Exports/Texture/UTextureProFX.h"
#include "UE4/Assets/Exports/Texture/UTextureRenderTarget.h"
#include "UE4/Assets/Exports/Texture/UTextureRenderTarget2D.h"
#include "UE4/Assets/Exports/Texture/UTextureRenderTargetCube.h"
#include "UE4/Assets/Exports/Texture/UVirtualTexture2D.h"
#include "UE4/Assets/Exports/Texture/UVolumeTexture.h"

// The prerequisites this slice pulled in.
#include "UE4/Assets/Exports/Component/IAssetUserData.h"
#include "UE4/Assets/Exports/Material/CMaterialParams.h"
#include "UE4/Assets/Exports/Material/UUnrealMaterial.h"
#include "UE4/Assets/Objects/FEditorBulkData.h"
#include "UE4/Objects/Core/Compression/FCompressedBuffer.h"
#include "UE4/Objects/Engine/UAssetUserData.h"

using namespace CUE4Parse::UE4::Assets;
using namespace CUE4Parse::UE4::Assets::Exports;
using namespace CUE4Parse::UE4::Assets::Exports::Texture;
using namespace CUE4Parse::UE4::Objects::UObject;
using namespace CUE4Parse::UE4::Readers;
using namespace CUE4Parse::UE4::Versions;
using CUE4Parse::UE4::Assets::Objects::EBulkDataFlags;

static int g_failures = 0;
#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::cerr << "FAIL: " << #cond << " (line " << __LINE__ << ")\n";  \
            ++g_failures;                                                      \
        }                                                                      \
    } while (0)

// ---- 1. compile-time shape of the tree -------------------------------------------------------------

static_assert(std::is_base_of_v<Material::UUnrealMaterial, UTexture>);
static_assert(std::is_base_of_v<Component::IAssetUserData, UTexture>);
static_assert(std::is_base_of_v<UObject, UTexture>);
static_assert(std::is_base_of_v<UTexture, UTexture2D>);
static_assert(std::is_base_of_v<UTexture, UTextureCube>);
static_assert(std::is_base_of_v<UTexture, UTextureCubeArray>);
static_assert(std::is_base_of_v<UTexture, UTexture2DArray>);
static_assert(std::is_base_of_v<UTexture, UVolumeTexture>);
static_assert(std::is_base_of_v<UTexture, UMediaTexture>);
static_assert(std::is_base_of_v<UTexture, UBinkMediaTexture>);
static_assert(std::is_base_of_v<UTexture, UTextureMovie>);
static_assert(std::is_base_of_v<UTexture, UTextureRenderTarget>);
static_assert(std::is_base_of_v<UTextureRenderTarget, UTextureRenderTarget2D>);
static_assert(std::is_base_of_v<UTextureRenderTarget, UTextureRenderTargetCube>);
static_assert(std::is_base_of_v<UTexture, UTextureProFXParent>);
static_assert(std::is_base_of_v<UTexture, UTextureProFXChild>);
// The 2D family. ULightMapTexture2D and UTextureLightProfile are the two that add wire/property data.
static_assert(std::is_base_of_v<UTexture2D, ULightMapTexture2D>);
static_assert(std::is_base_of_v<UTexture2D, UTextureLightProfile>);
static_assert(std::is_base_of_v<UTexture2D, UCurveLinearColorAtlas>);
static_assert(std::is_base_of_v<UTexture2D, ULightMapVirtualTexture2D>);
static_assert(std::is_base_of_v<UTexture2D, URuntimeVirtualTextureStreamingProxy>);
static_assert(std::is_base_of_v<UTexture2D, UShadowMapTexture2D>);
static_assert(std::is_base_of_v<UTexture2D, USubstanceAirTexture2D>);
static_assert(std::is_base_of_v<UTexture2D, UTerrainWeightMapTexture>);
static_assert(std::is_base_of_v<UTexture2D, UTextureFlipBook>);
static_assert(std::is_base_of_v<UTexture2D, UVirtualTexture2D>);
// UPaperSprite lives in this folder but is NOT a texture -- it only points at one.
static_assert(std::is_base_of_v<UObject, UPaperSprite>);
static_assert(!std::is_base_of_v<UTexture, UPaperSprite>);
// The mip-data providers are asset user data, not textures.
static_assert(std::is_base_of_v<CUE4Parse::UE4::Objects::Engine::UAssetUserData, UTextureMipDataProviderFactory>);
static_assert(std::is_base_of_v<UTextureMipDataProviderFactory, UTextureAllMipDataProviderFactory>);
// UTexture is abstract in C# only by convention; here GetParams has a body, so what must hold is that the
// whole tree stays polymorphic-deletable through UObject.
static_assert(std::has_virtual_destructor_v<UObject>);
// A mip owns its bulk data, so it must be move-only (a copy would double-own the payload).
static_assert(!std::is_copy_constructible_v<FTexture2DMipMap>);
static_assert(std::is_move_constructible_v<FTexture2DMipMap>);

// ---- 2. pure logic ---------------------------------------------------------------------------------

static void TestPixelFormatTable()
{
    const auto& formats = PixelFormatUtils::PixelFormats();

    // Every listed row must agree with the key it is filed under, or a lookup returns another format's
    // geometry -- the kind of table typo that only shows up as garbled pixels.
    for (const auto& entry : formats)
        CHECK(entry.second.UnrealFormat == entry.first);

    const FPixelFormatInfo* dxt5 = PixelFormatUtils::TryGetPixelFormatInfo(EPixelFormat::PF_DXT5);
    CHECK(dxt5 != nullptr);
    CHECK(dxt5->BlockSizeX == 4 && dxt5->BlockSizeY == 4 && dxt5->BlockBytes == 16);
    CHECK(dxt5->Supported);
    // 8x8 DXT5 = 2x2 blocks of 16 bytes.
    CHECK(dxt5->Get2DImageSizeInBytes(8, 8) == 64);
    // A 5-pixel row still costs two blocks: the round-up is the whole point of GetBlockCountForWidth.
    CHECK(dxt5->GetBlockCountForWidth(5) == 2);
    CHECK(dxt5->GetBlockCountForHeight(1) == 1);
    // Mip chain 8x8 -> 4x4 -> 2x2 -> 1x1; every level below 4x4 still costs one whole block.
    CHECK(dxt5->Get2DTextureMipSizeInBytes(8, 8, 1) == 16);
    CHECK(dxt5->Get2DTextureMipSizeInBytes(8, 8, 3) == 16);
    CHECK(dxt5->Get2DTextureSizeInBytes(8, 8, 4) == 64 + 16 + 16 + 16);

    const FPixelFormatInfo* bgra = PixelFormatUtils::TryGetPixelFormatInfo(EPixelFormat::PF_B8G8R8A8);
    CHECK(bgra != nullptr && bgra->Get2DImageSizeInBytes(4, 4) == 64); // 1x1 blocks of 4 bytes

    // PF_Unknown's row is all zeroes, so every block count is 0 rather than a divide by zero.
    const FPixelFormatInfo* unknown = PixelFormatUtils::TryGetPixelFormatInfo(EPixelFormat::PF_Unknown);
    CHECK(unknown != nullptr && unknown->GetBlockCountForWidth(64) == 0);
    CHECK(unknown->Get2DImageSizeInBytes(64, 64) == 0);

    // A format the table does not list at all (PF_MAX is a sentinel, never a real row).
    CHECK(PixelFormatUtils::TryGetPixelFormatInfo(EPixelFormat::PF_MAX) == nullptr);

    CHECK(PixelFormatUtils::IsHDR(EPixelFormat::PF_BC6H));
    CHECK(PixelFormatUtils::IsHDR(EPixelFormat::PF_ASTC_12x12_HDR));
    CHECK(!PixelFormatUtils::IsHDR(EPixelFormat::PF_DXT5));
    CHECK(!PixelFormatUtils::IsHDR(EPixelFormat::PF_ASTC_12x12));
}

static void TestPixelFormatParsing()
{
    EPixelFormat f = EPixelFormat::PF_MAX;

    CHECK(PixelFormatUtils::TryParsePixelFormat("PF_DXT5", f) && f == EPixelFormat::PF_DXT5);
    // C# passes ignoreCase: true, and cooked names are not consistently cased.
    CHECK(PixelFormatUtils::TryParsePixelFormat("pf_dxt5", f) && f == EPixelFormat::PF_DXT5);
    CHECK(PixelFormatUtils::TryParsePixelFormat("PF_astc_4x4_HDR", f) && f == EPixelFormat::PF_ASTC_4x4_HDR);
    // The three custom formats past the PF_MAX sentinel.
    CHECK(PixelFormatUtils::TryParsePixelFormat("PF_ASTC_10x8", f) && f == EPixelFormat::PF_ASTC_10x8);
    // Enum.TryParse also accepts the underlying value spelled out.
    CHECK(PixelFormatUtils::TryParsePixelFormat("7", f) && f == EPixelFormat::PF_DXT5);
    // ... and refuses anything else, leaving the caller's value untouched.
    f = EPixelFormat::PF_BC7;
    CHECK(!PixelFormatUtils::TryParsePixelFormat("PF_NotAFormat", f));
    CHECK(f == EPixelFormat::PF_BC7);
    CHECK(!PixelFormatUtils::TryParsePixelFormat("", f));

    // Every format the geometry table knows must also be nameable, or a cooked texture whose format name is
    // on disk would fall back to PF_Unknown and read its mips with the wrong block size.
    for (const auto& entry : PixelFormatUtils::PixelFormats())
    {
        EPixelFormat roundTrip = EPixelFormat::PF_MAX;
        const std::string numeric = std::to_string(static_cast<int>(entry.first));
        CHECK(PixelFormatUtils::TryParsePixelFormat(numeric, roundTrip) && roundTrip == entry.first);
    }
}

static void TestPackedDataBitMasks()
{
    FTexturePlatformData pd;

    pd.PackedData = 1u;
    CHECK(pd.GetNumSlices() == 1);
    CHECK(!pd.IsCubemap() && !pd.HasOptData() && !pd.HasCpuCopy());

    pd.PackedData = 6u | (1u << 31);          // a cubemap: six slices
    CHECK(pd.IsCubemap());
    CHECK(pd.GetNumSlices() == 6);
    CHECK(!pd.HasOptData());

    pd.PackedData = 1u | (1u << 30);          // opt data present
    CHECK(pd.HasOptData() && !pd.IsCubemap() && !pd.HasCpuCopy());
    CHECK(pd.GetNumSlices() == 1);

    pd.PackedData = 1u | (1u << 29);          // 5.4+ CPU copy
    CHECK(pd.HasCpuCopy() && !pd.HasOptData());

    // FAITHFUL QUIRK: the slice mask is BitMask_HasOptData - 1, i.e. everything below bit 30 -- which
    // still INCLUDES the HasCpuCopy bit 29. C# has exactly this mask, so a texture with a CPU copy reports
    // its slice count with bit 29 set. Kept rather than "fixed": UE only ever writes small slice counts,
    // and diverging here would make the port disagree with the C# on the same bytes.
    pd.PackedData = 6u | (1u << 31) | (1u << 30) | (1u << 29);
    CHECK(pd.GetNumSlices() == static_cast<int>(6u | (1u << 29)));
    pd.PackedData = 6u | (1u << 31) | (1u << 30);
    CHECK(pd.GetNumSlices() == 6);
    pd.PackedData = 6u | (1u << 31) | (1u << 30) | (1u << 29);
    CHECK(pd.IsCubemap() && pd.HasOptData() && pd.HasCpuCopy());

    pd.OptData.NumMipsInTail = 3;
    pd.OptData.ExtData = 42;
    CHECK(pd.GetNumMipsInTail() == 3);
    CHECK(pd.GetExtData() == 42);

    // A default-constructed one is what UTexture starts with: no mips, nothing serialized.
    const FTexturePlatformData fresh;
    CHECK(fresh.FirstMipToSerialize == -1);
    CHECK(fresh.Mips.empty());
    CHECK(!fresh.VTData.has_value());
    CHECK(fresh.PixelFormat.empty());
}

static void TestVirtualTextureTileOffsetLoop()
{
    // C#'s GetTileOffset is an open-coded Algo::UpperBound - 1 with a quirk: blockIndex is only assigned on
    // the FIRST address strictly greater than the query, and the "past the end" arm only fires when nothing
    // has been assigned yet. Both halves are pinned here because the loop is easy to "clean up" into
    // something that behaves differently.
    FVirtualTextureTileOffsetData d(4, 4, 16);
    d.Addresses = {0, 10, 20};
    d.Offsets = {100, 200, 300};

    CHECK(d.GetTileOffset(0) == 100);   // block 0, local offset 0
    CHECK(d.GetTileOffset(5) == 105);   // block 0, local offset 5
    CHECK(d.GetTileOffset(10) == 200);  // exactly the second address -> block 1
    CHECK(d.GetTileOffset(15) == 205);
    CHECK(d.GetTileOffset(25) == 305);  // past every address -> last block

    // ~0u in the offset table means "no data here", and propagates rather than being added to.
    FVirtualTextureTileOffsetData empty(4, 4, 16);
    empty.Addresses = {0};
    empty.Offsets = {~0u};
    CHECK(empty.GetTileOffset(3) == ~0u);

    // The three-argument constructor is the legacy synthesised one: no address table at all.
    CHECK(empty.Width == 4 && empty.Height == 4 && empty.MaxAddress == 16);
}

static void TestVirtualTextureBuiltDataAccessors()
{
    FVirtualTextureBuiltData vt;
    // A default one is not initialized -- UTexture::GetMipIndexByMaxSize keys off exactly this.
    CHECK(!vt.IsInitialized());

    vt.TileSize = 128;
    vt.TileBorderSize = 4;
    vt.Width = 1000;
    vt.Height = 256;
    CHECK(vt.IsInitialized());
    CHECK(vt.GetPhysicalTileSize() == 136);          // tile + both borders
    CHECK(vt.GetWidthInTiles() == 8);                // 1000 / 128 rounded up
    CHECK(vt.GetHeightInTiles() == 2);

    // No legacy tile table -> the UE5 layout, and GetChunkIndex answers -1 until ChunkIndexPerMip is read.
    CHECK(!vt.IsLegacyData());
    CHECK(vt.GetNumTileHeaders() == 0);
    CHECK(vt.GetChunkIndex(0) == -1);

    vt.ChunkIndexPerMip = {0, 0, 1};
    CHECK(vt.GetChunkIndex(2) == 1);
    CHECK(vt.GetChunkIndex(3) == -1);                // past the end, not a crash

    vt.TileOffsetInChunk = {0, 64};
    CHECK(vt.IsLegacyData());
    CHECK(vt.GetNumTileHeaders() == 2);
}

// ---- 3. an end-to-end cooked Texture2D -------------------------------------------------------------

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

static void AppendFName(std::vector<uint8_t>& buf, int32_t nameIndex, int32_t extraIndex = 0)
{
    AppendLE<int32_t>(buf, nameIndex);
    AppendLE<int32_t>(buf, extraIndex);
}

// One cooked mip: the "cooked" bool, an inline bulk-data header, the payload itself, then the dimensions.
static void AppendMip(std::vector<uint8_t>& buf, int32_t sizeX, int32_t sizeY,
                      const std::vector<uint8_t>& payload)
{
    AppendLE<int32_t>(buf, 1); // cooked (ReadBoolean is an int32)

    const uint32_t flags = static_cast<uint32_t>(EBulkDataFlags::BULKDATA_ForceInlinePayload) |
                           static_cast<uint32_t>(EBulkDataFlags::BULKDATA_NoOffsetFixUp);
    AppendLE<uint32_t>(buf, flags);
    AppendLE<int32_t>(buf, static_cast<int32_t>(payload.size()));   // ElementCount
    AppendLE<uint32_t>(buf, static_cast<uint32_t>(payload.size())); // SizeOnDisk
    AppendLE<int64_t>(buf, 0);                                     // OffsetInFile (inline, so unused)
    buf.insert(buf.end(), payload.begin(), payload.end());

    AppendLE<int32_t>(buf, sizeX);
    AppendLE<int32_t>(buf, sizeY);
    AppendLE<int32_t>(buf, 1); // SizeZ
}

static void TestCookedTexture2DRoundTrip()
{
    const int32_t UE4_AUTO = static_cast<int32_t>(EUnrealEngineObjectUE4Version::AUTOMATIC_VERSION);
    const VersionContainer VC(GAME_UE4_LATEST, ETexturePlatform::DesktopMobile, FPackageFileVersion(864, UE4_AUTO, 0));

    const std::vector<std::string> pool =
        {"None", "Core", "Class", "Engine", "Texture2D", "MyTexture", "MyPackage", "PF_DXT5"};
    enum : int32_t { N_None = 0, N_Core, N_Class, N_Engine, N_Texture2D, N_MyTexture, N_MyPackage, N_PF_DXT5 };
    const int32_t nameCount = static_cast<int32_t>(pool.size());
    const int32_t importCount = 2;
    const int32_t exportCount = 1;

    auto buildNames = [&] {
        std::vector<uint8_t> b;
        for (const auto& s : pool) { AppendFString(b, s); AppendLE<uint32_t>(b, 0); }
        return b;
    };

    // import[0] is the export's class: "Texture2D", which is what ObjectTypeRegistry keys on.
    auto buildImports = [&] {
        std::vector<uint8_t> b;
        AppendFName(b, N_Core); AppendFName(b, N_Class); AppendLE<int32_t>(b, -2); AppendFName(b, N_Texture2D); AppendFName(b, N_MyPackage);
        AppendFName(b, N_Core); AppendFName(b, N_Class); AppendLE<int32_t>(b, 0);  AppendFName(b, N_Engine);    AppendFName(b, N_MyPackage);
        return b;
    };

    auto buildExports = [&](int64_t serialOffset, int64_t serialSize) {
        std::vector<uint8_t> b;
        AppendLE<int32_t>(b, -1);           // ClassIndex -> import[0] "Texture2D"
        AppendLE<int32_t>(b, 0);            // SuperIndex
        AppendLE<int32_t>(b, 0);            // TemplateIndex
        AppendLE<int32_t>(b, 0);            // OuterIndex
        AppendFName(b, N_MyTexture);
        AppendLE<uint32_t>(b, 1u);          // ObjectFlags (RF_Public)
        AppendLE<int64_t>(b, serialSize);
        AppendLE<int64_t>(b, serialOffset);
        AppendLE<int32_t>(b, 0); AppendLE<int32_t>(b, 0); AppendLE<int32_t>(b, 0);
        AppendLE<uint32_t>(b, 0); AppendLE<uint32_t>(b, 0); AppendLE<uint32_t>(b, 0); AppendLE<uint32_t>(b, 0);
        AppendLE<uint32_t>(b, 0);
        AppendLE<int32_t>(b, 1); AppendLE<int32_t>(b, 1);
        AppendLE<int32_t>(b, -1);
        AppendLE<int32_t>(b, 0); AppendLE<int32_t>(b, 0); AppendLE<int32_t>(b, 0); AppendLE<int32_t>(b, 0);
        return b;
    };

    // 8x8 and 4x4 DXT5 mips: 2x2 and 1x1 blocks of 16 bytes. The sizes are the ones the format table
    // predicts, which is checked below.
    std::vector<uint8_t> mip0(64), mip1(16);
    for (size_t i = 0; i < mip0.size(); i++) mip0[i] = static_cast<uint8_t>(0xA0 + i);
    for (size_t i = 0; i < mip1.size(); i++) mip1[i] = static_cast<uint8_t>(0x10 + i);

    // The export body. skipOffset is an absolute file offset, so it is patched once the serial offset is
    // known; skipOffsetAt/platformDataEnd record where to patch and what to patch it to.
    size_t skipOffsetAt = 0;
    size_t platformDataEnd = 0;
    auto buildExportData = [&] {
        std::vector<uint8_t> b;
        AppendFName(b, N_None);       // no tagged properties at all -- every field takes its default
        AppendLE<int32_t>(b, 0);      // ObjectGuid present? false

        b.push_back(1); b.push_back(0); // UTexture's FStripDataFlags: editor data stripped
        b.push_back(0); b.push_back(0); // UTexture2D's FStripDataFlags
        AppendLE<int32_t>(b, 1);        // bCooked

        AppendFName(b, N_PF_DXT5);      // the pixel format block's name
        skipOffsetAt = b.size();
        AppendLE<int64_t>(b, 0);        // skipOffset, patched below

        // ---- FTexturePlatformData ----
        AppendLE<int32_t>(b, 0);        // SizeX  (overwritten from mip 0)
        AppendLE<int32_t>(b, 0);        // SizeY
        AppendLE<uint32_t>(b, 1u);      // PackedData: one slice, no flags
        AppendFString(b, "PF_DXT5");    // PixelFormat
        AppendLE<int32_t>(b, 0);        // FirstMipToSerialize
        AppendLE<int32_t>(b, 2);        // mip count
        AppendMip(b, 8, 8, mip0);
        AppendMip(b, 4, 4, mip1);
        AppendLE<int32_t>(b, 0);        // bIsVirtual (VirtualTextures is on from UE 4.23)
        // ---- end FTexturePlatformData ----

        platformDataEnd = b.size();
        AppendFName(b, N_None);         // terminates the per-format loop
        return b;
    };

    const int32_t summaryLen = [&] {
        std::vector<uint8_t> b;
        AppendLE<uint32_t>(b, FPackageFileSummary::PACKAGE_FILE_TAG);
        AppendLE<int32_t>(b, -7); AppendLE<int32_t>(b, 864); AppendLE<int32_t>(b, UE4_AUTO);
        AppendLE<int32_t>(b, 0); AppendLE<int32_t>(b, 0); AppendLE<int32_t>(b, 4096);
        AppendFString(b, "MyPackage");
        AppendLE<uint32_t>(b, 0);
        AppendLE<int32_t>(b, 0); AppendLE<int32_t>(b, 0);
        AppendFString(b, "");
        AppendLE<int32_t>(b, 0); AppendLE<int32_t>(b, 0);
        AppendLE<int32_t>(b, 0); AppendLE<int32_t>(b, 0);
        AppendLE<int32_t>(b, 0); AppendLE<int32_t>(b, 0);
        AppendLE<int32_t>(b, 0); AppendLE<int32_t>(b, 0); AppendLE<int32_t>(b, 0);
        AppendLE<int32_t>(b, 0); AppendLE<int32_t>(b, 0);
        for (int i = 0; i < 8; i++) AppendLE<uint32_t>(b, 0);
        AppendLE<int32_t>(b, 0);
        AppendLE<int32_t>(b, 0); AppendLE<int32_t>(b, 0);
        AppendLE<int32_t>(b, 0); AppendLE<int32_t>(b, 0);
        AppendLE<int64_t>(b, 0);
        AppendLE<int32_t>(b, 0); AppendLE<int32_t>(b, 0);
        AppendLE<int32_t>(b, 0); AppendLE<int32_t>(b, 0);
        return static_cast<int32_t>(b.size());
    }();

    // Rebuild the summary properly now that its length is known.
    auto buildSummary = [&](int32_t nameOffset, int32_t importOffset, int32_t exportOffset) {
        std::vector<uint8_t> b;
        AppendLE<uint32_t>(b, FPackageFileSummary::PACKAGE_FILE_TAG);
        AppendLE<int32_t>(b, -7);
        AppendLE<int32_t>(b, 864);
        AppendLE<int32_t>(b, UE4_AUTO);
        AppendLE<int32_t>(b, 0);            // FileVersionLicenseeUE
        AppendLE<int32_t>(b, 0);            // CustomVersion count
        AppendLE<int32_t>(b, 4096);         // TotalHeaderSize
        AppendFString(b, "MyPackage");
        AppendLE<uint32_t>(b, 0);           // PackageFlags
        AppendLE<int32_t>(b, nameCount);
        AppendLE<int32_t>(b, nameOffset);
        AppendFString(b, "");               // LocalizationId
        AppendLE<int32_t>(b, 0); AppendLE<int32_t>(b, 0);      // GatherableTextData
        AppendLE<int32_t>(b, exportCount); AppendLE<int32_t>(b, exportOffset);
        AppendLE<int32_t>(b, importCount); AppendLE<int32_t>(b, importOffset);
        AppendLE<int32_t>(b, 0);            // DependsOffset
        AppendLE<int32_t>(b, 0); AppendLE<int32_t>(b, 0);      // SoftPackageReferences
        AppendLE<int32_t>(b, 0);            // SearchableNamesOffset
        AppendLE<int32_t>(b, 0);            // ThumbnailTableOffset
        for (int i = 0; i < 8; i++) AppendLE<uint32_t>(b, 0);  // Guid + PersistentGuid
        AppendLE<int32_t>(b, 0);            // Generations count
        AppendLE<uint16_t>(b, 4); AppendLE<uint16_t>(b, 27); AppendLE<uint16_t>(b, 2);
        AppendLE<uint32_t>(b, 0); AppendFString(b, "++UE4+Release-4.27");
        AppendLE<uint16_t>(b, 4); AppendLE<uint16_t>(b, 27); AppendLE<uint16_t>(b, 2);
        AppendLE<uint32_t>(b, 0); AppendFString(b, "++UE4+Release-4.27");
        AppendLE<int32_t>(b, 0);            // CompressionFlags
        AppendLE<int32_t>(b, 0);            // compressedChunks count
        AppendLE<int32_t>(b, 0);            // PackageSource
        AppendLE<int32_t>(b, 0);            // additionalPackagesToCook count
        AppendLE<int32_t>(b, 0);            // AssetRegistryDataOffset
        AppendLE<int64_t>(b, 0);            // BulkDataStartOffset
        AppendLE<int32_t>(b, 0);            // WorldTileInfoDataOffset
        AppendLE<int32_t>(b, 0);            // ChunkIds count
        AppendLE<int32_t>(b, 0);            // PreloadDependencyCount
        AppendLE<int32_t>(b, 0);            // PreloadDependencyOffset
        return b;
    };
    (void) summaryLen;

    const int32_t realSummaryLen = static_cast<int32_t>(buildSummary(0, 0, 0).size());
    const std::vector<uint8_t> names = buildNames();
    const std::vector<uint8_t> imports = buildImports();
    std::vector<uint8_t> exportData = buildExportData();

    const int32_t nameOffset = realSummaryLen;
    const int32_t importOffset = nameOffset + static_cast<int32_t>(names.size());
    const int32_t exportOffset = importOffset + static_cast<int32_t>(imports.size());
    const int32_t exportSectionLen = static_cast<int32_t>(buildExports(0, 0).size());
    const int32_t serialOffset = exportOffset + exportSectionLen;

    // Patch the skip offset: it is the absolute file position just past the platform data.
    const int64_t skipOffset = serialOffset + static_cast<int64_t>(platformDataEnd);
    std::memcpy(exportData.data() + skipOffsetAt, &skipOffset, sizeof(int64_t));

    const std::vector<uint8_t> exports = buildExports(serialOffset, static_cast<int64_t>(exportData.size()));

    std::vector<uint8_t> buf = buildSummary(nameOffset, importOffset, exportOffset);
    buf.insert(buf.end(), names.begin(), names.end());
    buf.insert(buf.end(), imports.begin(), imports.end());
    buf.insert(buf.end(), exports.begin(), exports.end());
    buf.insert(buf.end(), exportData.begin(), exportData.end());

    FByteArchive uasset("MyPackage.uasset", buf, VC);
    Package pkg(uasset);

    CHECK(pkg.ExportMap.size() == 1);
    UObject* obj = pkg.GetExportObject(0);
    CHECK(obj != nullptr);
    if (obj == nullptr) return;

    // The registry must have picked UTexture2D off the "Texture2D" class import.
    auto* tex = dynamic_cast<UTexture2D*>(obj);
    CHECK(tex != nullptr);
    if (tex == nullptr) return;

    CHECK(tex->Name == "MyTexture");
    CHECK(tex->Format == EPixelFormat::PF_DXT5);
    CHECK(tex->PlatformData.PixelFormat == "PF_DXT5");

    // SizeX/SizeY come off mip 0, NOT off the header (which held zeroes) -- that overwrite is the easiest
    // thing to lose in translation, and losing it makes every texture report 0x0.
    CHECK(tex->PlatformData.SizeX == 8);
    CHECK(tex->PlatformData.SizeY == 8);
    CHECK(tex->PlatformData.GetNumSlices() == 1);
    CHECK(!tex->PlatformData.IsCubemap());
    CHECK(tex->PlatformData.FirstMipToSerialize == 0);
    CHECK(!tex->PlatformData.VTData.has_value());

    CHECK(tex->PlatformData.Mips.size() == 2);
    if (tex->PlatformData.Mips.size() == 2)
    {
        CHECK(tex->PlatformData.Mips[0].SizeX == 8 && tex->PlatformData.Mips[0].SizeY == 8);
        CHECK(tex->PlatformData.Mips[1].SizeX == 4 && tex->PlatformData.Mips[1].SizeY == 4);
        CHECK(tex->PlatformData.Mips[0].SizeZ == 1);

        // The payload sizes must be exactly what the format table predicts for those dimensions.
        const FPixelFormatInfo* dxt5 = PixelFormatUtils::TryGetPixelFormatInfo(EPixelFormat::PF_DXT5);
        CHECK(dxt5 != nullptr);
        CHECK(tex->PlatformData.Mips[0].BulkData != nullptr);
        CHECK(tex->PlatformData.Mips[1].BulkData != nullptr);
        if (dxt5 != nullptr && tex->PlatformData.Mips[0].BulkData != nullptr)
        {
            CHECK(tex->PlatformData.Mips[0].BulkData->GetDataSize() == dxt5->Get2DImageSizeInBytes(8, 8));
            CHECK(tex->PlatformData.Mips[1].BulkData->GetDataSize() == dxt5->Get2DImageSizeInBytes(4, 4));

            // And the bytes themselves come back out of the package's export archive.
            const std::vector<uint8_t>* data = tex->PlatformData.Mips[0].BulkData->Data();
            CHECK(data != nullptr);
            if (data != nullptr)
            {
                CHECK(data->size() == mip0.size());
                CHECK(*data == mip0);
            }
        }
    }

    // Defaults: no property was written, so every one of these is the fallback UTexture picks. TF_Nearest as
    // the default Filter is not the engine's own default -- it is CUE4Parse's, and it is load-bearing for
    // RenderNearestNeighbor.
    CHECK(tex->SRGB);
    CHECK(tex->Filter == TextureFilter::TF_Nearest);
    CHECK(tex->LODGroup == TextureGroup::TEXTUREGROUP_World);
    CHECK(tex->CompressionSettings == TextureCompressionSettings::TC_Default);
    CHECK(tex->RenderNearestNeighbor());
    CHECK(!tex->IsNormalMap());
    CHECK(!tex->IsHDR());
    CHECK(tex->AssetUserData.empty());
    CHECK(tex->MipDataProvider() == nullptr);
    CHECK(tex->GetTextureAddressX() == tex->AddressX);

    // The mip accessors, now that there is real bulk data behind them.
    CHECK(tex->GetFirstMipIndex() == 0);
    CHECK(tex->GetFirstMip() == &tex->PlatformData.Mips[0]);
    CHECK(tex->GetMip(1) == &tex->PlatformData.Mips[1]);
    CHECK(tex->GetMip(2) == nullptr);
    CHECK(tex->GetMip(-1) == nullptr);
    CHECK(tex->GetMipBySize(4, 4) == &tex->PlatformData.Mips[1]);
    CHECK(tex->GetMipByMaxSize(4) == &tex->PlatformData.Mips[1]);
    CHECK(tex->GetMipIndexByMaxSize(8) == 0);
    // Nothing is small enough, so it falls back to the first valid mip rather than failing.
    CHECK(tex->GetMipByMaxSize(1) == &tex->PlatformData.Mips[0]);
}

int main()
{
    TestPixelFormatTable();
    TestPixelFormatParsing();
    TestPackedDataBitMasks();
    TestVirtualTextureTileOffsetLoop();
    TestVirtualTextureBuiltDataAccessors();
    TestCookedTexture2DRoundTrip();

    if (g_failures == 0) std::cout << "test_texture: all checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
