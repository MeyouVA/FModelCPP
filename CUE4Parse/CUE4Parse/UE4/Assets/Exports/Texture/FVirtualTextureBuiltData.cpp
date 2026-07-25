#include "FVirtualTextureBuiltData.h"

#include <stdexcept>

namespace CUE4Parse::UE4::Assets::Exports::Texture
{
    using namespace CUE4Parse::UE4::Versions;

    FVirtualTextureBuiltData::FVirtualTextureBuiltData(Readers::FAssetArchive& Ar, int firstMip)
    {
        (void) firstMip; // C#: var bStripMips = firstMip > 0; -- commented out there, so unused here too.
        Ar.ReadBoolean(); // bCooked

        NumLayers = Ar.Read<uint32_t>();
        // C#: Debug.Assert(NumLayers <= 8u) -- VIRTUALTEXTURE_DATA_MAXLAYERS. See the header note.
        WidthInBlocks = Ar.Read<uint32_t>();
        HeightInBlocks = Ar.Read<uint32_t>();
        TileSize = Ar.Read<uint32_t>();
        TileBorderSize = Ar.Read<uint32_t>();
        if (Ar.Game() >= GAME_UE5_0) TileDataOffsetPerLayer = Ar.ReadArrayCounted<uint32_t>();

        //if (!bStripMips)
        {
            NumMips = Ar.Read<uint32_t>();
            Width = Ar.Read<uint32_t>();
            Height = Ar.Read<uint32_t>();

            if (Ar.Game() >= GAME_UE5_0)
            {
                ChunkIndexPerMip = Ar.ReadArrayCounted<uint32_t>();
                BaseOffsetPerMip = Ar.ReadArrayCounted<uint32_t>();
                TileOffsetData = Ar.ReadArrayWith([&Ar] { return FVirtualTextureTileOffsetData(Ar); });
            }

            TileIndexPerChunk = Ar.ReadArrayCounted<uint32_t>();
            TileIndexPerMip = Ar.ReadArrayCounted<uint32_t>();
            TileOffsetInChunk = Ar.ReadArrayCounted<uint32_t>();
        }

        LayerTypes = Ar.ReadArrayWith(static_cast<int>(NumLayers), [&Ar]
        {
            const std::string name = Ar.ReadFString();
            EPixelFormat format = EPixelFormat::PF_Unknown;
            // C# uses Enum.Parse here (not TryParse), which throws on a name it does not know.
            if (!PixelFormatUtils::TryParsePixelFormat(name, format))
                throw std::runtime_error("Requested value '" + name + "' was not found");
            return format;
        });

        if (Ar.Game() >= GAME_UE5_0)
        {
            LayerFallbackColors.resize(NumLayers);
            for (uint32_t i = 0; i < NumLayers; i++)
            {
                LayerFallbackColors[i] = Ar.Read<FLinearColor>();
            }
        }

        const uint32_t numLayers = NumLayers;
        Chunks = Ar.ReadArrayWith([&Ar, numLayers] { return FVirtualTextureDataChunk(Ar, numLayers); });
    }

    int FVirtualTextureBuiltData::GetChunkIndex_Legacy(uint32_t tileIndex) const
    {
        const int max = static_cast<int>(Chunks.size()) - 1;
        // C#'s `TileIndexPerChunk != null && tileIndex <= TileIndexPerChunk.Last()`: Last() on an empty
        // array throws there, so the emptiness check below is the port refusing to do the same.
        if (!TileIndexPerChunk.empty() && tileIndex <= TileIndexPerChunk.back())
        {
            for (int i = 0; i < max; i++)
            {
                const size_t idx = static_cast<size_t>(i);
                if (tileIndex >= TileIndexPerChunk[idx] && tileIndex < TileIndexPerChunk[idx + 1])
                    return i;
            }
        }

        return max;
    }

    uint32_t FVirtualTextureBuiltData::GetTileIndex_Legacy(int vLevel, uint32_t vAddress) const
    {
        // C#: Debug.Assert(vLevel < NumMips).
        const uint32_t tileIndex = TileIndexPerMip[static_cast<size_t>(vLevel)] + vAddress * NumLayers;
        if (tileIndex >= TileIndexPerMip[static_cast<size_t>(vLevel) + 1])
        {
            // vAddress is out of bounds for this texture/mip level
            return ~0u;
        }
        return tileIndex;
    }

    uint32_t FVirtualTextureBuiltData::GetTileOffset_Legacy(int chunkIndex, uint32_t tileIndex) const
    {
        // C#: Debug.Assert(tileIndex >= TileIndexPerChunk[chunkIndex]).
        if (tileIndex < TileIndexPerChunk[static_cast<size_t>(chunkIndex) + 1])
        {
            return TileOffsetInChunk[tileIndex];
        }

        // If TileIndex is past the end of chunk, return the size of chunk
        // This allows us to determine size of region by asking for start/end offsets
        return Chunks[static_cast<size_t>(chunkIndex)].SizeInBytes;
    }

    bool FVirtualTextureBuiltData::IsValidAddress(int vLevel, uint32_t vAddress) const
    {
        bool bIsValid = false;
        if (IsLegacyData())
        {
            bIsValid = GetTileIndex_Legacy(vLevel, vAddress) != ~0u;
        }
        else
        {
            if (vLevel < static_cast<int>(TileOffsetData.size()))
            {
                const uint32_t x = CUE4Parse::Utils::ReverseMortonCode2(vAddress);
                const uint32_t y = CUE4Parse::Utils::ReverseMortonCode2(vAddress >> 1);
                bIsValid = x < TileOffsetData[static_cast<size_t>(vLevel)].Width &&
                           y < TileOffsetData[static_cast<size_t>(vLevel)].Height;
            }
        }

        return bIsValid;
    }

    FVirtualTextureBuiltData::FTileData FVirtualTextureBuiltData::GetTileData(int vLevel, uint32_t vAddress,
                                                                             uint32_t layerIndex) const
    {
        FTileData result;

        if (IsLegacyData())
        {
            const uint32_t tileIndex = GetTileIndex_Legacy(vLevel, vAddress);
            if (tileIndex != ~0u)
            {
                // If size of the tile is 0 we return ~0u to indicate that there is no data present.
                result.ChunkIndex = GetChunkIndex_Legacy(tileIndex);
                const uint32_t tileOffset = GetTileOffset_Legacy(result.ChunkIndex, tileIndex);
                const uint32_t nextTileOffset = GetTileOffset_Legacy(result.ChunkIndex, tileIndex + NumLayers);
                if (tileOffset != nextTileOffset)
                {
                    result.Offset = GetTileOffset_Legacy(result.ChunkIndex, tileIndex + layerIndex);
                    result.TileDataLength =
                        GetTileOffset_Legacy(result.ChunkIndex, tileIndex + layerIndex + 1) - result.Offset;
                }
            }
        }
        else if (static_cast<int>(BaseOffsetPerMip.size()) > vLevel && static_cast<int>(TileOffsetData.size()) > vLevel)
        {
            // If the tile offset is ~0u there is no data present so we return ~0u to indicate that.
            result.ChunkIndex = GetChunkIndex(vLevel);
            const uint32_t baseOffset = BaseOffsetPerMip[static_cast<size_t>(vLevel)];
            const uint32_t tileOffset = TileOffsetData[static_cast<size_t>(vLevel)].GetTileOffset(vAddress);
            if (baseOffset != ~0u && tileOffset != ~0u)
            {
                // C#: Debug.Assert(TileDataOffsetPerLayer != null).
                const uint32_t tileDataSize = TileDataOffsetPerLayer.back();
                result.TileDataLength = layerIndex == 0 ? 0 : TileDataOffsetPerLayer[layerIndex - 1];

                result.Offset = baseOffset + (tileOffset * tileDataSize) + result.TileDataLength;
            }
        }

        return result;
    }

    FVirtualTextureTileOffsetData FVirtualTextureBuiltData::GetTileOffsetData(int level) const
    {
        if (!IsLegacyData()) return TileOffsetData[static_cast<size_t>(level)];

        // calculate the max address in this mip
        // aka get the next mip max address and subtract it by the current mip max address
        const uint32_t blockWidthInTiles = GetWidthInTiles();
        const uint32_t blockHeightInTiles = GetHeightInTiles();
        const uint32_t nextLevel = static_cast<uint32_t>(level) + 1 < NumMips
            ? static_cast<uint32_t>(level) + 1
            : NumMips; // Math.Min(level + 1, NumMips)
        const uint32_t maxAddress = TileIndexPerMip[nextLevel];
        const uint32_t current = TileIndexPerMip[static_cast<size_t>(level)];
        return FVirtualTextureTileOffsetData(blockWidthInTiles, blockHeightInTiles,
                                             maxAddress - current > 1 ? maxAddress - current : 1);
    }
}
