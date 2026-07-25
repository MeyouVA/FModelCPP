// Ported from CUE4Parse/UE4/Assets/Exports/Texture/UTexture.cs
// The base every texture type derives from. Two things happen here: the tagged properties common to all
// textures are lifted out, and DeserializeCookedPlatformData walks the per-pixel-format blocks a cooked
// package writes -- one block per format the cook produced, of which only the FIRST is actually read and
// the rest are skipped via their own skip offsets.
//
// Deliberate differences from C#:
//   * LightingGuid's default. C# is `new FGuid((uint) GetFullName().GetHashCode())`. GetFullName is not
//     ported (it needs the object-loading path), and .NET Core randomises string.GetHashCode per process,
//     so that "default" is not even stable across two runs of the C#. A default FGuid is used instead --
//     the value is only ever reached when the asset has no LightingGuid property at all. TODO if the
//     exporter layer ever needs the same bytes.
//   * WriteJson is dropped (the port has no JSON serializer layer).
//   * GetParams stays pure-virtual-with-an-empty-override here exactly as in C#: both bodies are empty and
//     commented "???" there too.
//   * The mip-data provider is cached in a mutable member so MipDataProvider() stays a const-ish accessor,
//     matching C#'s lazily-filled backing field.
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "ETextureCookPlatformTilingSettings.h"
#include "FTexturePlatformData.h"
#include "PixelFormat.h"
#include "TextureAddress.h"
#include "TextureCompressionSettings.h"
#include "TextureFilter.h"
#include "TextureGroup.h"
#include "UTextureAllMipDataProviderFactory.h"
#include "../Component/IAssetUserData.h"
#include "../Material/UUnrealMaterial.h"
#include "../../Objects/FByteBulkData.h"
#include "../../Objects/FEditorBulkData.h"
#include "../../Readers/FAssetArchive.h"
#include "../../../Objects/Core/Misc/FGuid.h"
#include "../../../Objects/UObject/ObjectResource.h"

namespace CUE4Parse::UE4::Assets::Exports::Texture
{
    using CUE4Parse::UE4::Assets::Exports::Component::IAssetUserData;
    using CUE4Parse::UE4::Assets::Exports::Material::CMaterialParams;
    using CUE4Parse::UE4::Assets::Exports::Material::CMaterialParams2;
    using CUE4Parse::UE4::Assets::Exports::Material::EMaterialFormat;
    using CUE4Parse::UE4::Assets::Exports::Material::UUnrealMaterial;
    using CUE4Parse::UE4::Assets::Objects::FByteBulkData;
    using CUE4Parse::UE4::Assets::Objects::FEditorBulkData;
    using CUE4Parse::UE4::Objects::Core::Misc::FGuid;
    using CUE4Parse::UE4::Objects::UObject::FPackageIndex;

    class UTexture : public UUnrealMaterial, public IAssetUserData
    {
    public:
        FGuid LightingGuid;
        TextureCompressionSettings CompressionSettings = TextureCompressionSettings::TC_Default;
        TextureGroup LODGroup = TextureGroup::TEXTUREGROUP_World;
        TextureFilter Filter = TextureFilter::TF_Nearest;
        bool SRGB = true;
        std::vector<FPackageIndex> AssetUserData;
        EPixelFormat Format = EPixelFormat::PF_Unknown;
        FTexturePlatformData PlatformData;
        std::optional<FEditorBulkData> EditorData;
        std::optional<FByteBulkData> SourceArt;
        ETextureCookPlatformTilingSettings CookPlatformTilingSettings{};

        const std::vector<FPackageIndex>& GetAssetUserData() const override { return AssetUserData; }

        bool RenderNearestNeighbor() const
        {
            return LODGroup == TextureGroup::TEXTUREGROUP_Pixels2D || Filter == TextureFilter::TF_Nearest;
        }
        bool IsNormalMap() const { return CompressionSettings == TextureCompressionSettings::TC_Normalmap; }
        bool IsHDR() const
        {
            switch (CompressionSettings)
            {
                case TextureCompressionSettings::TC_HDR:
                case TextureCompressionSettings::TC_HDR_F32:
                case TextureCompressionSettings::TC_HDR_Compressed:
                case TextureCompressionSettings::TC_HalfFloat:
                case TextureCompressionSettings::TC_SingleFloat:
                    return true;
                default:
                    return false;
            }
        }

        virtual TextureAddress GetTextureAddressX() const { return TextureAddress::TA_Wrap; }
        virtual TextureAddress GetTextureAddressY() const { return TextureAddress::TA_Wrap; }
        virtual TextureAddress GetTextureAddressZ() const { return TextureAddress::TA_Wrap; }

        // C#'s lazily-resolved MipDataProvider property: the first AssetUserData entry that loads as an
        // all-mip provider factory. Null when there is none, which is the usual case.
        UTextureAllMipDataProviderFactory* MipDataProvider();

        void Deserialize(Readers::FAssetArchive& Ar, int64_t validPos) override;

        FTexture2DMipMap* GetMip(int index);
        FTexture2DMipMap* GetFirstMip();
        int GetFirstMipIndex();
        int GetMipIndexByMaxSize(int maxXSize, int maxYSize = -1);
        FTexture2DMipMap* GetMipByMaxSize(int maxSize);
        FTexture2DMipMap* GetMipBySize(int sizeX, int sizeY);

        void GetParams(CMaterialParams& parameters) override
        {
            (void) parameters;
            // Default empty method
            // ???
        }

        void GetParams(CMaterialParams2& parameters, EMaterialFormat format) override
        {
            (void) parameters;
            (void) format;
            // Default empty method
            // ???
        }

    protected:
        void DeserializeCookedPlatformData(Readers::FAssetArchive& Ar, bool bSerializeMipData = true);

    private:
        UTextureAllMipDataProviderFactory* _mipDataProvider = nullptr;
    };

    class UBinkMediaTexture : public UTexture
    {
    };
}
