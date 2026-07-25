#include "UTexture.h"

#include "../PropertyUtil.h"
#include "../../ResolvedObject.h"
#include "../../../Objects/Engine/FStripDataFlags.h"
#include "../../../Versions/EGame.h"
#include "../../../Versions/FUE5MainStreamObjectVersion.h"

namespace CUE4Parse::UE4::Assets::Exports::Texture
{
    using CUE4Parse::UE4::Objects::Engine::FStripDataFlags;
    using CUE4Parse::UE4::Objects::UObject::FName;
    using namespace CUE4Parse::UE4::Versions;

    UTextureAllMipDataProviderFactory* UTexture::MipDataProvider()
    {
        if (_mipDataProvider == nullptr)
        {
            for (const FPackageIndex& aud : AssetUserData)
            {
                // C#'s aud.TryLoad<UTextureAllMipDataProviderFactory>(out _mipDataProvider): resolve the
                // index against its package, load, and keep it only if it is of that type.
                Assets::ResolvedObject* resolved =
                    aud.Owner != nullptr ? aud.Owner->ResolvePackageIndex(&aud) : nullptr;
                if (resolved == nullptr) continue;
                if (auto* factory = resolved->Load<UTextureAllMipDataProviderFactory>())
                {
                    _mipDataProvider = factory;
                    break;
                }
            }
        }
        return _mipDataProvider;
    }

    void UTexture::Deserialize(Readers::FAssetArchive& Ar, int64_t validPos)
    {
        if (Ar.Game() == GAME_WorldofJadeDynasty || Ar.Game() == GAME_RocoKingdomWorld) Ar.Position += 16;
        UObject::Deserialize(Ar, validPos);
        // See the header note: C#'s default here is a hash of the full name, which is neither reachable nor
        // stable in this port.
        LightingGuid = PropertyUtil::GetOrDefault<FGuid>(*this, "LightingGuid");
        CompressionSettings = PropertyUtil::GetOrDefault<TextureCompressionSettings>(
            *this, "CompressionSettings", TextureCompressionSettings::TC_Default);
        LODGroup = PropertyUtil::GetOrDefault<TextureGroup>(*this, "LODGroup", TextureGroup::TEXTUREGROUP_World);
        Filter = PropertyUtil::GetOrDefault<TextureFilter>(*this, "Filter", TextureFilter::TF_Nearest);
        SRGB = PropertyUtil::GetOrDefault<bool>(*this, "SRGB", true);
        AssetUserData = PropertyUtil::GetArray<FPackageIndex>(*this, "AssetUserData");
        CookPlatformTilingSettings = PropertyUtil::GetOrDefault<ETextureCookPlatformTilingSettings>(
            *this, "CookPlatformTilingSettings");

        if (Ar.Game() < GAME_UE4_0)
        {
            SourceArt.emplace(Ar);
            return;
        }

        const FStripDataFlags stripFlags(Ar);

        // If archive is has editor only data
        if (!stripFlags.IsEditorDataStripped())
        {
            if (FUE5MainStreamObjectVersion::Get(Ar) < FUE5MainStreamObjectVersion::VirtualizedBulkDataHaveUniqueGuids)
            {
                if (FUE5MainStreamObjectVersion::Get(Ar) < FUE5MainStreamObjectVersion::TextureSourceVirtualization)
                {
                    FByteBulkData(Ar); // read purely for its effect on the cursor, as C#'s `new FByteBulkData(Ar);`
                }
                else
                {
                    EditorData.emplace(Ar);
                }
            }
            else
            {
                EditorData.emplace(Ar);
            }
        }
    }

    void UTexture::DeserializeCookedPlatformData(Readers::FAssetArchive& Ar, bool bSerializeMipData)
    {
        FName pixelFormatName = Ar.ReadFName();
        if (pixelFormatName.Text() == "PF_BC6H_Signed") pixelFormatName = FName(std::string("PF_BC6H"));
        while (!pixelFormatName.IsNone())
        {
            EPixelFormat pixelFormat = EPixelFormat::PF_Unknown;
            if (!PixelFormatUtils::TryParsePixelFormat(pixelFormatName.Text(), pixelFormat))
            {
                // C#: Log.Warning("Failed to parse pixel format: {PixelFormat}", ...). The port has no
                // logging layer; as in C#, pixelFormat stays PF_Unknown and the block is read anyway.
            }

            const int64_t skipOffset =
                Ar.Game() >= GAME_UE5_0  ? Ar.AbsolutePosition() + Ar.Read<int64_t>() :
                Ar.Game() >= GAME_UE4_20 ? Ar.Read<int64_t>() :
                                           static_cast<int64_t>(Ar.Read<int32_t>());

            if (Format == EPixelFormat::PF_Unknown)
            {
                //?? check whether we can support this pixel format
                PlatformData = FTexturePlatformData(Ar, *this, bSerializeMipData);

                if (Ar.Game() == GAME_SeaOfThieves || Ar.Game() == GAME_DeltaForce) Ar.Position += 4;

                if (Ar.AbsolutePosition() != skipOffset)
                {
                    // C# logs "Texture2D read incorrectly. Offset ..., Skip Offset ..., Bytes remaining ..."
                    // and then seeks anyway; the seek is the part that matters and is kept.
                    Ar.SeekAbsolute(skipOffset, CUE4Parse::UE4::Readers::ESeekOrigin::Begin);
                }

                Format = pixelFormat;
            }
            else
            {
                // C#: Log.Debug("Skipping data for format {Format}", pixelFormatName) -- DEBUG builds only.
                Ar.SeekAbsolute(skipOffset, CUE4Parse::UE4::Readers::ESeekOrigin::Begin);
            }

            pixelFormatName = Ar.ReadFName();
        }
    }

    FTexture2DMipMap* UTexture::GetMip(int index)
    {
        return index >= 0 && index < static_cast<int>(PlatformData.Mips.size()) &&
               PlatformData.Mips[static_cast<size_t>(index)].EnsureValidBulkData(MipDataProvider(), index)
            ? &PlatformData.Mips[static_cast<size_t>(index)]
            : nullptr;
    }

    FTexture2DMipMap* UTexture::GetFirstMip()
    {
        for (size_t i = 0; i < PlatformData.Mips.size(); i++)
        {
            if (PlatformData.Mips[i].EnsureValidBulkData(MipDataProvider(), static_cast<int>(i)))
                return &PlatformData.Mips[i];
        }
        return nullptr;
    }

    int UTexture::GetFirstMipIndex()
    {
        for (size_t i = 0; i < PlatformData.Mips.size(); i++)
        {
            if (PlatformData.Mips[i].EnsureValidBulkData(MipDataProvider(), static_cast<int>(i)))
                return static_cast<int>(i);
        }

        return -1;
    }

    int UTexture::GetMipIndexByMaxSize(int maxXSize, int maxYSize)
    {
        if (maxYSize == -1)
            maxYSize = maxXSize;

        if (PlatformData.FirstMipToSerialize >= 0 && PlatformData.VTData.has_value() &&
            PlatformData.VTData->IsInitialized())
        {
            const FVirtualTextureBuiltData& vt = *PlatformData.VTData;
            const uint32_t tileSize = vt.TileSize;
            for (int i = 0; i < static_cast<int>(vt.NumMips); i++)
            {
                const FVirtualTextureTileOffsetData tileOffsetData = vt.GetTileOffsetData(i);
                if (static_cast<int>(tileOffsetData.Width * tileSize) <= maxXSize ||
                    static_cast<int>(tileOffsetData.Height * tileSize) <= maxYSize)
                    return i;
            }
            return -1;
        }

        for (size_t i = 0; i < PlatformData.Mips.size(); i++)
        {
            FTexture2DMipMap& mip = PlatformData.Mips[i];
            if ((mip.SizeX <= maxXSize || mip.SizeY <= maxYSize) &&
                mip.EnsureValidBulkData(MipDataProvider(), static_cast<int>(i)))
                return static_cast<int>(i);
        }

        return GetFirstMipIndex();
    }

    FTexture2DMipMap* UTexture::GetMipByMaxSize(int maxSize)
    {
        for (size_t i = 0; i < PlatformData.Mips.size(); i++)
        {
            FTexture2DMipMap& mip = PlatformData.Mips[i];
            if ((mip.SizeX <= maxSize || mip.SizeY <= maxSize) &&
                mip.EnsureValidBulkData(MipDataProvider(), static_cast<int>(i)))
                return &mip;
        }

        return GetFirstMip();
    }

    FTexture2DMipMap* UTexture::GetMipBySize(int sizeX, int sizeY)
    {
        for (size_t i = 0; i < PlatformData.Mips.size(); i++)
        {
            FTexture2DMipMap& mip = PlatformData.Mips[i];
            if (mip.SizeX == sizeX && mip.SizeY == sizeY &&
                mip.EnsureValidBulkData(MipDataProvider(), static_cast<int>(i)))
                return &mip;
        }

        return GetFirstMip();
    }
}
