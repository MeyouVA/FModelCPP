// Ported from CUE4Parse/UE4/Assets/Exports/Texture/FTexture2DMipMap.cs
// One mip level: a bulk-data handle plus its dimensions. The bulk data is not read here -- FByteBulkData
// stays lazy, so a texture can be walked for its geometry without touching a single payload byte.
//
// Deliberate differences from C#:
//   * The [JsonConverter] attribute is dropped (no JSON serializer layer in this port).
//   * BulkData is a unique_ptr<TBulkData<uint8_t>> rather than a nullable reference. It has to be a pointer
//     because EnsureValidBulkData can *replace* an FByteBulkData with an FByteArrayData, i.e. the concrete
//     type varies at runtime; C# gets that for free from reference semantics.
//   * EnsureValidBulkData's landscape arm is not reachable yet: ULandscapeTextureStorageProviderFactory is
//     not ported, so the provider is always the plain factory and the method returns false -- which is what
//     C# does for every provider that is not a landscape one. TODO with the Landscape tree.
#pragma once

#include <cstdint>
#include <memory>
#include <utility>

#include "../../Objects/FByteBulkData.h"
#include "../../Readers/FAssetArchive.h"
#include "../../../Versions/EGame.h"
#include "../../../Versions/ObjectVersion.h"

namespace CUE4Parse::UE4::Assets::Exports::Texture
{
    using CUE4Parse::UE4::Assets::Objects::TBulkData;
    using CUE4Parse::UE4::Assets::Objects::FByteBulkData;
    using CUE4Parse::UE4::Versions::EUnrealEngineObjectUE4Version;
    using namespace CUE4Parse::UE4::Versions;

    class UTextureAllMipDataProviderFactory;

    class FTexture2DMipMap
    {
    public:
        std::unique_ptr<TBulkData<uint8_t>> BulkData;
        int32_t SizeX = 0;
        int32_t SizeY = 0;
        int32_t SizeZ = 0;

        FTexture2DMipMap() = default;

        FTexture2DMipMap(std::unique_ptr<TBulkData<uint8_t>> bulkData, int32_t sizeX, int32_t sizeY, int32_t sizeZ)
            : BulkData(std::move(bulkData)), SizeX(sizeX), SizeY(sizeY), SizeZ(sizeZ) {}

        explicit FTexture2DMipMap(Readers::FAssetArchive& Ar, bool bSerializeMipData = true)
        {
            const bool cooked = Ar.Ver() >= EUnrealEngineObjectUE4Version::TEXTURE_SOURCE_ART_REFACTOR &&
                                Ar.Game() < GAME_UE5_0
                                ? Ar.ReadBoolean()
                                : Ar.IsFilterEditorOnly();

            if (bSerializeMipData) BulkData = std::make_unique<FByteBulkData>(Ar);

            if (Ar.Game() == GAME_Borderlands3)
            {
                SizeX = Ar.Read<uint16_t>();
                SizeY = Ar.Read<uint16_t>();
                SizeZ = Ar.Read<uint16_t>();
            }
            else if (Ar.Game() == GAME_WorldofJadeDynasty)
            {
                SizeX = static_cast<int32_t>(Ar.Read<uint32_t>() ^ 0xa537ea93u);
                SizeY = Ar.Read<int32_t>();
                SizeZ = static_cast<int32_t>(ReverseEndianness(Ar.Read<uint32_t>()));
            }
            else
            {
                SizeX = Ar.Read<int32_t>();
                SizeY = Ar.Read<int32_t>();
                SizeZ = Ar.Game() >= GAME_UE4_20 ? Ar.Read<int32_t>() : 1;
            }

            if (Ar.Ver() >= EUnrealEngineObjectUE4Version::TEXTURE_DERIVED_DATA2 && !cooked)
            {
                if (Ar.Game() >= GAME_UE4_26) Ar.Read<uint8_t>();   // FileRegionType
                if (Ar.Game() < GAME_UE5_0) Ar.ReadFString();       // derivedDataKey
                if (Ar.Game() >= GAME_UE5_0) Ar.ReadBoolean();      // bPagedToDerivedData
            }
        }

        // C#'s EnsureValidBulkData: true when there are bytes to work with, either already or after asking
        // the mip-data provider to produce them. See the header note about the landscape arm.
        bool EnsureValidBulkData(UTextureAllMipDataProviderFactory* provider, int mipLevel)
        {
            (void) provider;
            (void) mipLevel;

            if (BulkData != nullptr)
            {
                const std::vector<uint8_t>* data = BulkData->Data();
                if (data != nullptr && !data->empty()) return true;
            }

            return false;
        }

    private:
        // BinaryPrimitives.ReverseEndianness, local as in FModGuid.h / FCompressedBufferHeader.h.
        static uint32_t ReverseEndianness(uint32_t v)
        {
            return (v >> 24) | ((v >> 8) & 0x0000FF00u) | ((v << 8) & 0x00FF0000u) | (v << 24);
        }
    };
}
