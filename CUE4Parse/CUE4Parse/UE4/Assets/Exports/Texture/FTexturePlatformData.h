// Ported from CUE4Parse/UE4/Assets/Exports/Texture/FTexturePlatformData.cs
// The cooked side of a texture: dimensions, the packed slice/flag word, the mip chain, and -- when the
// texture is virtual -- the tile index instead. Note that SizeX/SizeY as read off the header are OVERWRITTEN
// by mip 0's dimensions when there is a mip chain; the header values only survive for a virtual texture.
//
// Deliberate differences from C#:
//   * Owner is a const UTexture& (forward-declared here, dereferenced in the .cpp) because C# passes the
//     texture in only to ask "are you a volume texture or a cubemap?" and to read one property off it.
//   * The readonly fields that the constructor nonetheless rewrites at the end (SizeX, PackedData) are just
//     plain members; C# gets away with `readonly` because the writes happen inside the constructor.
//   * FSharedImage's Mip is an FTexture2DMipMap holding an FByteArrayData built from bytes read eagerly --
//     C# does the same, so this is one of the few places in the tree where a payload really is materialised.
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "FTexture2DMipMap.h"
#include "FVirtualTextureBuiltData.h"
#include "PixelFormat.h"
#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Exports::Texture
{
    class UTexture;

    struct FOptTexturePlatformData
    {
        uint32_t ExtData = 0;
        uint32_t NumMipsInTail = 0;
    };

    // A CPU-side copy of the texture kept alongside the GPU mips (UE 5.4+).
    struct FSharedImage
    {
        int32_t SizeX = 0;
        int32_t SizeY = 0;
        int32_t SizeZ = 0;
        EPixelFormat Format = EPixelFormat::PF_Unknown;
        uint8_t GammaSpace = 0;
        FTexture2DMipMap Mip;

        explicit FSharedImage(Readers::FAssetArchive& Ar);
    };

    class FTexturePlatformData
    {
        static constexpr uint32_t BitMask_CubeMap    = 1u << 31;
        static constexpr uint32_t BitMask_HasOptData = 1u << 30;
        static constexpr uint32_t BitMask_HasCpuCopy = 1u << 29;
        static constexpr uint32_t BitMask_NumSlices  = BitMask_HasOptData - 1u;

    public:
        int32_t SizeX = 0;
        int32_t SizeY = 0;
        uint32_t PackedData = 0; // NumSlices: 1 for simple texture, 6 for cubemap - 6 textures are joined into one
        std::string PixelFormat;
        FOptTexturePlatformData OptData;
        int32_t FirstMipToSerialize = -1;
        std::vector<FTexture2DMipMap> Mips;
        std::optional<FVirtualTextureBuiltData> VTData;
        std::optional<FSharedImage> CPUCopy;

        FTexturePlatformData() = default;

        FTexturePlatformData(Readers::FAssetArchive& Ar, const UTexture& Owner, bool bSerializeMipData = true);

        bool HasCpuCopy() const { return (PackedData & BitMask_HasCpuCopy) == BitMask_HasCpuCopy; }
        bool HasOptData() const { return (PackedData & BitMask_HasOptData) == BitMask_HasOptData; }
        bool IsCubemap() const { return (PackedData & BitMask_CubeMap) == BitMask_CubeMap; }
        int GetNumSlices() const { return static_cast<int>(PackedData & BitMask_NumSlices); }
        int GetNumMipsInTail() const { return static_cast<int>(OptData.NumMipsInTail); }
        int GetExtData() const { return static_cast<int>(OptData.ExtData); }
    };
}
