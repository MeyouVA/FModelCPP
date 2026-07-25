// Ported from CUE4Parse/UE4/Assets/Exports/Texture/FVirtualTextureBuiltData.cs
// The tile index of a virtual texture: how the mip pyramid maps onto the streamed chunks. There are two
// layouts and the class serves both -- a legacy one keyed off flat per-tile offset arrays, and the UE5 one
// keyed off per-mip FVirtualTextureTileOffsetData with Morton-coded addresses. IsLegacyData picks.
//
// Deliberate differences from C#:
//   * GetTileData returns a small struct instead of a C# tuple.
//   * C#'s Debug.Assert calls are dropped: they are no-ops in a release build, which is the configuration
//     every caller of this class is compiled in. The arithmetic they guard is unchanged.
//   * Enum.Parse over the layer type strings goes through PixelFormatUtils::TryParsePixelFormat. C# would
//     THROW on an unknown name; this keeps that, since a layer type it cannot name means the rest of the
//     stream is misaligned anyway.
#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "FVirtualTextureDataChunk.h"
#include "PixelFormat.h"
#include "../../Readers/FAssetArchive.h"
#include "../../../Objects/Core/Math/FLinearColor.h"
#include "../../../Versions/EGame.h"
#include "../../../../Utils/MathUtils.h"

namespace CUE4Parse::UE4::Assets::Exports::Texture
{
    using CUE4Parse::UE4::Objects::Core::Math::FLinearColor;

    struct FVirtualTextureTileOffsetData
    {
        uint32_t Width = 0;
        uint32_t Height = 0;
        uint32_t MaxAddress = 0;
        std::vector<uint32_t> Addresses;
        std::vector<uint32_t> Offsets;

        FVirtualTextureTileOffsetData() = default;

        explicit FVirtualTextureTileOffsetData(Readers::FArchive& Ar)
        {
            Width = Ar.Read<uint32_t>();
            Height = Ar.Read<uint32_t>();
            MaxAddress = Ar.Read<uint32_t>();
            Addresses = Ar.ReadArrayCounted<uint32_t>();
            Offsets = Ar.ReadArrayCounted<uint32_t>();
        }

        FVirtualTextureTileOffsetData(uint32_t width, uint32_t height, uint32_t maxAddress)
            : Width(width), Height(height), MaxAddress(maxAddress) {}

        uint32_t GetTileOffset(uint32_t inAddress) const
        {
            // Algo::UpperBound(Addresses, InAddress) - 1, spelled out exactly as the C# spells it: the loop
            // only ever assigns blockIndex on the FIRST address past inAddress, and the second arm catches
            // the case where every address is <= inAddress.
            size_t blockIndex = 0;
            for (size_t i = 0; i < Addresses.size(); i++)
            {
                if (Addresses[i] > inAddress)
                {
                    blockIndex = i - 1;
                    break;
                }
                if (i == Addresses.size() - 1 && blockIndex == 0)
                {
                    blockIndex = Addresses.size() - 1;
                }
            }

            const uint32_t baseOffset = Offsets[blockIndex];
            if (baseOffset == ~0u) return ~0u;

            const uint32_t baseAddress = Addresses[blockIndex];
            const uint32_t localOffset = inAddress - baseAddress;
            return baseOffset + localOffset;
        }
    };

    class FVirtualTextureBuiltData
    {
    public:
        // What C#'s (int chunkIndex, uint offset, uint tileDataLength) tuple carries.
        struct FTileData
        {
            int ChunkIndex = 0;
            uint32_t Offset = 0;
            uint32_t TileDataLength = 0;
        };

        uint32_t NumLayers = 0;
        uint32_t NumMips = 0;
        uint32_t Width = 0;
        uint32_t Height = 0;
        uint32_t WidthInBlocks = 0;
        uint32_t HeightInBlocks = 0;
        uint32_t TileSize = 0;
        uint32_t TileBorderSize = 0;
        std::vector<EPixelFormat> LayerTypes;
        std::vector<FVirtualTextureDataChunk> Chunks;
        std::vector<uint32_t> TileIndexPerChunk;
        std::vector<uint32_t> TileIndexPerMip;
        std::vector<uint32_t> TileOffsetInChunk;
        std::vector<uint32_t> ChunkIndexPerMip;
        std::vector<uint32_t> BaseOffsetPerMip;
        std::vector<uint32_t> TileDataOffsetPerLayer;
        std::vector<FVirtualTextureTileOffsetData> TileOffsetData;
        std::vector<FLinearColor> LayerFallbackColors;

        // C# only has the reading constructor; the default one exists here so the type can sit in a
        // container/optional the way a C# null reference would. An un-read instance is exactly the
        // `!IsInitialized()` state C# reaches through a null VTData.
        FVirtualTextureBuiltData() = default;
        FVirtualTextureBuiltData(Readers::FAssetArchive& Ar, int firstMip);

        bool IsInitialized() const { return TileSize != 0; }
        uint32_t GetPhysicalTileSize() const { return TileSize + TileBorderSize * 2u; }
        uint32_t GetWidthInTiles() const { return CUE4Parse::Utils::DivideAndRoundUp(Width, TileSize); }
        uint32_t GetHeightInTiles() const { return CUE4Parse::Utils::DivideAndRoundUp(Height, TileSize); }

        // C#'s `TileOffsetInChunk == null || TileOffsetInChunk.Length > 0`. The port has no null vector, and
        // the only way TileOffsetInChunk is left unread is the (commented-out) bStripMips branch, so the
        // "null" half of that test can never fire here -- an empty vector means the legacy arrays WERE read
        // and were empty, which is the same answer C# gives for a zero-length array.
        bool IsLegacyData() const { return !TileOffsetInChunk.empty(); }
        int GetNumTileHeaders() const { return static_cast<int>(TileOffsetInChunk.size()); }

        int GetChunkIndex(int vLevel) const
        {
            return vLevel < static_cast<int>(ChunkIndexPerMip.size())
                ? static_cast<int>(ChunkIndexPerMip[static_cast<size_t>(vLevel)])
                : -1;
        }

        int GetChunkIndex_Legacy(uint32_t tileIndex) const;
        uint32_t GetTileIndex_Legacy(int vLevel, uint32_t vAddress) const;
        uint32_t GetTileOffset_Legacy(int chunkIndex, uint32_t tileIndex) const;
        bool IsValidAddress(int vLevel, uint32_t vAddress) const;
        FTileData GetTileData(int vLevel, uint32_t vAddress, uint32_t layerIndex) const;
        FVirtualTextureTileOffsetData GetTileOffsetData(int level) const;
    };
}
