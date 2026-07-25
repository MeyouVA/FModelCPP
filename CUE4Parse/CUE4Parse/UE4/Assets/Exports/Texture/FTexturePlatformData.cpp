#include "FTexturePlatformData.h"

#include <stdexcept>

#include "UTexture.h"
#include "UTextureCube.h"
#include "UVolumeTexture.h"
#include "../PropertyUtil.h"
#include "../../Objects/FByteBulkData.h"
#include "../../../Versions/EGame.h"

namespace CUE4Parse::UE4::Assets::Exports::Texture
{
    using CUE4Parse::UE4::Assets::Objects::FByteArrayData;
    using namespace CUE4Parse::UE4::Versions;

    FSharedImage::FSharedImage(Readers::FAssetArchive& Ar)
    {
        SizeX = Ar.Read<int32_t>();
        SizeY = Ar.Read<int32_t>();
        SizeZ = Ar.Read<int32_t>();
        Format = Ar.Read<EPixelFormat>();
        GammaSpace = Ar.Read<uint8_t>();
        std::vector<uint8_t> rawData = Ar.ReadArray<uint8_t>(static_cast<int>(Ar.Read<int64_t>()));

        Mip = FTexture2DMipMap(std::make_unique<FByteArrayData>(std::move(rawData)), SizeX, SizeY, SizeZ);
    }

    FTexturePlatformData::FTexturePlatformData(Readers::FAssetArchive& Ar, const UTexture& Owner, bool bSerializeMipData)
    {
        constexpr int64_t PlaceholderDerivedDataSize = 16;
        if (Ar.Game() >= GAME_UE5_2)
        {
            if (Ar.ReadFlag() && Ar.Game() != GAME_InfinityNikki) // bUsingDerivedData
                throw std::runtime_error("FTexturePlatformData deserialization using derived data is not implemented.");
            Ar.Position += PlaceholderDerivedDataSize - 1;
        }
        else if (Ar.Game() >= GAME_UE5_0 && Ar.IsFilterEditorOnly())
        {
            Ar.Position += PlaceholderDerivedDataSize;
        }

        if (Ar.Game() == GAME_InfinityNikki) Ar.Position += 4;

        if (Ar.Game() == GAME_PlayerUnknownsBattlegrounds)
        {
            SizeX = Ar.Read<int16_t>();
            SizeY = Ar.Read<int16_t>();
            const std::vector<uint8_t> data = Ar.ReadArray<uint8_t>(3); // int24
            PackedData = static_cast<uint32_t>(data[0] + (data[1] << 8) + (data[2] << 16));
        }
        else
        {
            SizeX = Ar.Read<int32_t>();
            SizeY = Ar.Read<int32_t>();
            PackedData = Ar.Read<uint32_t>();
        }

        PixelFormat = Ar.Game() == GAME_GearsOfWar4 ? Ar.ReadFName().Text() : Ar.ReadFString();

        if (Ar.Game() == GAME_DragonQuestXI) Ar.Position += 4;
        if (Ar.Game() == GAME_FinalFantasy7Remake && (PackedData & 0xffff) == 16384)
        {
            Ar.Read<int32_t>(); // unk0
            Ar.Read<int32_t>(); // unk1
            Ar.Read<int32_t>(); // mapNum
        }

        if (HasOptData())
        {
            if (Ar.Game() == GAME_MidnightSuns) Ar.Position += 4;
            if (Ar.Game() == GAME_Psychonauts2) Ar.Position += 24;
            OptData = Ar.Read<FOptTexturePlatformData>();
        }

        if (HasCpuCopy()) // 5.4+
        {
            CPUCopy.emplace(Ar);
        }

        FirstMipToSerialize = Ar.Read<int32_t>(); // only for cooked, but we don't read FTexturePlatformData for non-cooked textures

        const int32_t mipCount = Ar.Read<int32_t>();

        if (Ar.Game() == GAME_FinalFantasy7Remake)
        {
            FTexture2DMipMap firstMip(Ar);
            const int32_t val = Ar.Read<int32_t>();
            if (val != static_cast<int32_t>(PackedData))
            {
                // oh no
            }

            Ar.Position += 4;
        }

        if (Ar.Game() == GAME_DaysGone) Ar.Position += 8;

        // The two subclasses whose mips are stacked slices rather than a plain 2D image. C# tests
        // `Owner is UVolumeTexture or UTextureCube`; a dynamic_cast chain is the same question.
        const bool ownerIsVolume = dynamic_cast<const UVolumeTexture*>(&Owner) != nullptr;
        const bool ownerStacksSlices = ownerIsVolume || dynamic_cast<const UTextureCube*>(&Owner) != nullptr;

        Mips.reserve(static_cast<size_t>(mipCount > 0 ? mipCount : 0));
        for (int32_t i = 0; i < mipCount; i++)
        {
            Mips.emplace_back(Ar, bSerializeMipData);

            if (ownerStacksSlices)
            {
                int slices = GetNumSlices();
                if (Ar.Game() == GAME_Borderlands4) slices = slices != 1 ? slices >> 1 : 1;
                FTexture2DMipMap& mip = Mips.back();
                mip.SizeY *= slices;
                mip.SizeZ = mip.SizeZ == slices ? 1 : mip.SizeZ;
            }
        }

        if (Ar.Versions["VirtualTextures"])
        {
            const bool bIsVirtual = Ar.ReadBoolean();
            if (bIsVirtual)
            {
                const int32_t LODBias = PropertyUtil::GetOrDefault<int32_t>(Owner, "LODBias");
                VTData.emplace(Ar, FirstMipToSerialize - LODBias);
            }
        }

        if (Ar.Game() == GAME_AssaultFireFuture && Ar.ReadBoolean()) Ar.Position += 112;

        if (!Mips.empty())
        {
            SizeX = Mips[0].SizeX;
            SizeY = Mips[0].SizeY;

            if (ownerIsVolume)
                PackedData = static_cast<uint32_t>((static_cast<uint32_t>(Mips[0].SizeZ) & BitMask_NumSlices) |
                                                   (PackedData & ~BitMask_NumSlices));
        }
        else if (VTData.has_value())
        {
            SizeX = static_cast<int32_t>(VTData->Width);
            SizeY = static_cast<int32_t>(VTData->Height);
        }
    }
}
