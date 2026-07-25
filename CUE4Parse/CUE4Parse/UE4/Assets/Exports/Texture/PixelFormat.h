// Ported from CUE4Parse/UE4/Assets/Exports/Texture/PixelFormat.cs
// EPixelFormat plus the block-geometry table every texture reader and decoder consults: how wide a
// compressed block is, how many bytes it occupies, and whether CUE4Parse can decode it at all.
//
// Deliberate differences from C#:
//   * PixelFormatUtils.PixelFormats is a public static Dictionary; here it is a function returning a
//     reference to a Meyers-singleton map, so there is no static-initialisation order to reason about.
//   * C# reads a pixel format back from disk with Enum.TryParse/Enum.Parse, which is reflection over the
//     member names. C++ has no such table, so TryParsePixelFormat below is a generated case-insensitive
//     name -> value map covering every member. It also accepts a bare number, as Enum.TryParse does.
//   * The `record`'s value equality is not needed by any caller and is not reproduced.
#pragma once

#include <cstdint>
#include <map>
#include <string>

namespace CUE4Parse::UE4::Assets::Exports::Texture
{
    enum class EPixelFormat : uint8_t
    {
        PF_Unknown              = 0,
        PF_A32B32G32R32F        = 1,
        PF_B8G8R8A8             = 2,
        PF_G8                   = 3, // G8  means Gray/Grey , not Green , typically actually uses a red format with replication of R to RGB
        PF_G16                  = 4, // G16 means Gray/Grey like G8
        PF_DXT1                 = 5,
        PF_DXT3                 = 6,
        PF_DXT5                 = 7,
        PF_UYVY                 = 8,
        PF_FloatRGB             = 9,  // 16F
        PF_FloatRGBA            = 10, // 16F
        PF_DepthStencil         = 11,
        PF_ShadowDepth          = 12,
        PF_R32_FLOAT            = 13,
        PF_G16R16               = 14,
        PF_G16R16F              = 15,
        PF_G16R16F_FILTER       = 16,
        PF_G32R32F              = 17,
        PF_A2B10G10R10          = 18,
        PF_A16B16G16R16         = 19,
        PF_D24                  = 20,
        PF_R16F                 = 21,
        PF_R16F_FILTER          = 22,
        PF_BC5                  = 23,
        PF_V8U8                 = 24,
        PF_A1                   = 25,
        PF_FloatR11G11B10       = 26,
        PF_A8                   = 27,
        PF_R32_UINT             = 28,
        PF_R32_SINT             = 29,
        PF_PVRTC2               = 30,
        PF_PVRTC4               = 31,
        PF_R16_UINT             = 32,
        PF_R16_SINT             = 33,
        PF_R16G16B16A16_UINT    = 34,
        PF_R16G16B16A16_SINT    = 35,
        PF_R5G6B5_UNORM         = 36,
        PF_R8G8B8A8             = 37,
        PF_A8R8G8B8             = 38,
        PF_BC4                  = 39,
        PF_R8G8                 = 40,
        PF_ATC_RGB              = 41,   // Unsupported Format
        PF_ATC_RGBA_E           = 42,   // Unsupported Format
        PF_ATC_RGBA_I           = 43,   // Unsupported Format
        PF_X24_G8               = 44,   // Used for creating SRVs to alias a DepthStencil buffer to read Stencil. Don't use for creating textures.
        PF_ETC1                 = 45,   // Unsupported Format
        PF_ETC2_RGB             = 46,
        PF_ETC2_RGBA            = 47,
        PF_R32G32B32A32_UINT    = 48,
        PF_R16G16_UINT          = 49,
        PF_ASTC_4x4             = 50,   // 8.00 bpp
        PF_ASTC_6x6             = 51,   // 3.56 bpp
        PF_ASTC_8x8             = 52,   // 2.00 bpp
        PF_ASTC_10x10           = 53,   // 1.28 bpp
        PF_ASTC_12x12           = 54,   // 0.89 bpp
        PF_BC6H                 = 55,
        PF_BC7                  = 56,
        PF_R8_UINT              = 57,
        PF_L8                   = 58,
        PF_XGXR8                = 59,
        PF_R8G8B8A8_UINT        = 60,
        PF_R8G8B8A8_SNORM       = 61,
        PF_R16G16B16A16_UNORM   = 62,
        PF_R16G16B16A16_SNORM   = 63,
        PF_PLATFORM_HDR_0       = 64,
        PF_PLATFORM_HDR_1       = 65,   // Reserved.
        PF_PLATFORM_HDR_2       = 66,   // Reserved.
        PF_NV12                 = 67,
        PF_R32G32_UINT          = 68,
        PF_ETC2_R11_EAC         = 69,
        PF_ETC2_RG11_EAC        = 70,
        PF_R8                   = 71,
        PF_B5G5R5A1_UNORM       = 72,
        PF_ASTC_4x4_HDR         = 73,
        PF_ASTC_6x6_HDR         = 74,
        PF_ASTC_8x8_HDR         = 75,
        PF_ASTC_10x10_HDR       = 76,
        PF_ASTC_12x12_HDR       = 77,
        PF_G16R16_SNORM         = 78,
        PF_R8G8_UINT            = 79,
        PF_R32G32B32_UINT       = 80,
        PF_R32G32B32_SINT       = 81,
        PF_R32G32B32F           = 82,
        PF_R8_SINT              = 83,
        PF_R64_UINT             = 84,
        PF_R9G9B9EXP5           = 85,
        PF_P010                 = 86,
        PF_ASTC_4x4_NORM_RG     = 87, // RG format stored in LA endpoints for better precision (requires RHI support for texture swizzle)
        PF_ASTC_6x6_NORM_RG     = 88,
        PF_ASTC_8x8_NORM_RG     = 89,
        PF_ASTC_10x10_NORM_RG   = 90,
        PF_ASTC_12x12_NORM_RG   = 91,
        PF_R16G16_SINT          = 92,
        PF_R8G8B8               = 93,
        PF_MAX                  = 94,

        // Custom
        PF_ASTC_8x5             = 253,
        PF_ASTC_8x6             = 254,
        PF_ASTC_10x8            = 255,
    };

    struct FPixelFormatInfo
    {
        EPixelFormat UnrealFormat = EPixelFormat::PF_Unknown;
        std::string Name;
        int BlockSizeX = 0;
        int BlockSizeY = 0;
        int BlockSizeZ = 0;
        int BlockBytes = 0;
        int NumComponents = 0;
        bool Supported = false;

        FPixelFormatInfo() = default;

        FPixelFormatInfo(EPixelFormat unrealFormat, std::string name, int blockSizeX, int blockSizeY,
                         int blockSizeZ, int blockBytes, int numComponents, bool supported)
            : UnrealFormat(unrealFormat), Name(std::move(name)), BlockSizeX(blockSizeX), BlockSizeY(blockSizeY),
              BlockSizeZ(blockSizeZ), BlockBytes(blockBytes), NumComponents(numComponents), Supported(supported) {}

        int GetBlockCountForWidth(int width) const
        {
            if (BlockSizeX > 0) return (width + BlockSizeX - 1) / BlockSizeX;
            return 0;
        }

        int GetBlockCountForHeight(int height) const
        {
            if (BlockSizeY > 0) return (height + BlockSizeY - 1) / BlockSizeY;
            return 0;
        }

        int GetBlockCountForDepth(int depth) const
        {
            if (BlockSizeZ > 0) return (depth + BlockSizeZ - 1) / BlockSizeZ;
            return 0;
        }

        int Get2DImageSizeInBytes(int width, int height) const
        {
            const int blockWidth = GetBlockCountForWidth(width);
            const int blockHeight = GetBlockCountForHeight(height);
            return blockWidth * blockHeight * BlockBytes;
        }

        int Get2DTextureMipSizeInBytes(int width, int height, int mipIdx) const
        {
            const int mipWidth = (width >> mipIdx) > 1 ? (width >> mipIdx) : 1;
            const int mipHeight = (height >> mipIdx) > 1 ? (height >> mipIdx) : 1;
            return Get2DImageSizeInBytes(mipWidth, mipHeight);
        }

        int Get2DTextureSizeInBytes(int width, int height, int mipCount) const
        {
            int size = 0;
            int mipWidth = width;
            int mipHeight = height;
            for (int idx = 0; idx < mipCount; ++idx)
            {
                size += Get2DImageSizeInBytes(mipWidth, mipHeight);
                mipWidth = (mipWidth >> 1) > 1 ? (mipWidth >> 1) : 1;
                mipHeight = (mipHeight >> 1) > 1 ? (mipHeight >> 1) : 1;
            }
            return size;
        }
    };

    namespace PixelFormatUtils
    {
        // PixelFormat.h IsHDR
        bool IsHDR(EPixelFormat pixelFormat);

        // C#'s public static Dictionary, behind an accessor (see the header note).
        const std::map<EPixelFormat, FPixelFormatInfo>& PixelFormats();

        // Convenience for the common "look it up or fall back to PF_Unknown's row" -- C# call sites index the
        // dictionary directly, which throws on a format the table does not list.
        const FPixelFormatInfo* TryGetPixelFormatInfo(EPixelFormat pixelFormat);

        // Stands in for C#'s Enum.TryParse<EPixelFormat>(text, ignoreCase: true, out value).
        bool TryParsePixelFormat(const std::string& text, EPixelFormat& outFormat);
    }
}
