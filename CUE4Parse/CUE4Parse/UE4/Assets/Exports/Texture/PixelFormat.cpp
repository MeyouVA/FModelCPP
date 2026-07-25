#include "PixelFormat.h"

#include <cctype>
#include <cstdlib>
#include <utility>
#include <vector>

namespace CUE4Parse::UE4::Assets::Exports::Texture
{
    namespace
    {
        // The enum member names, needed because C# gets them from reflection (Enum.TryParse / Enum.Parse) and
        // C++ cannot. Kept in declaration order so it reads against the enum in PixelFormat.h.
        const std::vector<std::pair<const char*, EPixelFormat>>& MemberNames()
        {
            static const std::vector<std::pair<const char*, EPixelFormat>> names = {
                { "PF_Unknown",              EPixelFormat::PF_Unknown },
                { "PF_A32B32G32R32F",        EPixelFormat::PF_A32B32G32R32F },
                { "PF_B8G8R8A8",             EPixelFormat::PF_B8G8R8A8 },
                { "PF_G8",                   EPixelFormat::PF_G8 },
                { "PF_G16",                  EPixelFormat::PF_G16 },
                { "PF_DXT1",                 EPixelFormat::PF_DXT1 },
                { "PF_DXT3",                 EPixelFormat::PF_DXT3 },
                { "PF_DXT5",                 EPixelFormat::PF_DXT5 },
                { "PF_UYVY",                 EPixelFormat::PF_UYVY },
                { "PF_FloatRGB",             EPixelFormat::PF_FloatRGB },
                { "PF_FloatRGBA",            EPixelFormat::PF_FloatRGBA },
                { "PF_DepthStencil",         EPixelFormat::PF_DepthStencil },
                { "PF_ShadowDepth",          EPixelFormat::PF_ShadowDepth },
                { "PF_R32_FLOAT",            EPixelFormat::PF_R32_FLOAT },
                { "PF_G16R16",               EPixelFormat::PF_G16R16 },
                { "PF_G16R16F",              EPixelFormat::PF_G16R16F },
                { "PF_G16R16F_FILTER",       EPixelFormat::PF_G16R16F_FILTER },
                { "PF_G32R32F",              EPixelFormat::PF_G32R32F },
                { "PF_A2B10G10R10",          EPixelFormat::PF_A2B10G10R10 },
                { "PF_A16B16G16R16",         EPixelFormat::PF_A16B16G16R16 },
                { "PF_D24",                  EPixelFormat::PF_D24 },
                { "PF_R16F",                 EPixelFormat::PF_R16F },
                { "PF_R16F_FILTER",          EPixelFormat::PF_R16F_FILTER },
                { "PF_BC5",                  EPixelFormat::PF_BC5 },
                { "PF_V8U8",                 EPixelFormat::PF_V8U8 },
                { "PF_A1",                   EPixelFormat::PF_A1 },
                { "PF_FloatR11G11B10",       EPixelFormat::PF_FloatR11G11B10 },
                { "PF_A8",                   EPixelFormat::PF_A8 },
                { "PF_R32_UINT",             EPixelFormat::PF_R32_UINT },
                { "PF_R32_SINT",             EPixelFormat::PF_R32_SINT },
                { "PF_PVRTC2",               EPixelFormat::PF_PVRTC2 },
                { "PF_PVRTC4",               EPixelFormat::PF_PVRTC4 },
                { "PF_R16_UINT",             EPixelFormat::PF_R16_UINT },
                { "PF_R16_SINT",             EPixelFormat::PF_R16_SINT },
                { "PF_R16G16B16A16_UINT",    EPixelFormat::PF_R16G16B16A16_UINT },
                { "PF_R16G16B16A16_SINT",    EPixelFormat::PF_R16G16B16A16_SINT },
                { "PF_R5G6B5_UNORM",         EPixelFormat::PF_R5G6B5_UNORM },
                { "PF_R8G8B8A8",             EPixelFormat::PF_R8G8B8A8 },
                { "PF_A8R8G8B8",             EPixelFormat::PF_A8R8G8B8 },
                { "PF_BC4",                  EPixelFormat::PF_BC4 },
                { "PF_R8G8",                 EPixelFormat::PF_R8G8 },
                { "PF_ATC_RGB",              EPixelFormat::PF_ATC_RGB },
                { "PF_ATC_RGBA_E",           EPixelFormat::PF_ATC_RGBA_E },
                { "PF_ATC_RGBA_I",           EPixelFormat::PF_ATC_RGBA_I },
                { "PF_X24_G8",               EPixelFormat::PF_X24_G8 },
                { "PF_ETC1",                 EPixelFormat::PF_ETC1 },
                { "PF_ETC2_RGB",             EPixelFormat::PF_ETC2_RGB },
                { "PF_ETC2_RGBA",            EPixelFormat::PF_ETC2_RGBA },
                { "PF_R32G32B32A32_UINT",    EPixelFormat::PF_R32G32B32A32_UINT },
                { "PF_R16G16_UINT",          EPixelFormat::PF_R16G16_UINT },
                { "PF_ASTC_4x4",             EPixelFormat::PF_ASTC_4x4 },
                { "PF_ASTC_6x6",             EPixelFormat::PF_ASTC_6x6 },
                { "PF_ASTC_8x8",             EPixelFormat::PF_ASTC_8x8 },
                { "PF_ASTC_10x10",           EPixelFormat::PF_ASTC_10x10 },
                { "PF_ASTC_12x12",           EPixelFormat::PF_ASTC_12x12 },
                { "PF_BC6H",                 EPixelFormat::PF_BC6H },
                { "PF_BC7",                  EPixelFormat::PF_BC7 },
                { "PF_R8_UINT",              EPixelFormat::PF_R8_UINT },
                { "PF_L8",                   EPixelFormat::PF_L8 },
                { "PF_XGXR8",                EPixelFormat::PF_XGXR8 },
                { "PF_R8G8B8A8_UINT",        EPixelFormat::PF_R8G8B8A8_UINT },
                { "PF_R8G8B8A8_SNORM",       EPixelFormat::PF_R8G8B8A8_SNORM },
                { "PF_R16G16B16A16_UNORM",   EPixelFormat::PF_R16G16B16A16_UNORM },
                { "PF_R16G16B16A16_SNORM",   EPixelFormat::PF_R16G16B16A16_SNORM },
                { "PF_PLATFORM_HDR_0",       EPixelFormat::PF_PLATFORM_HDR_0 },
                { "PF_PLATFORM_HDR_1",       EPixelFormat::PF_PLATFORM_HDR_1 },
                { "PF_PLATFORM_HDR_2",       EPixelFormat::PF_PLATFORM_HDR_2 },
                { "PF_NV12",                 EPixelFormat::PF_NV12 },
                { "PF_R32G32_UINT",          EPixelFormat::PF_R32G32_UINT },
                { "PF_ETC2_R11_EAC",         EPixelFormat::PF_ETC2_R11_EAC },
                { "PF_ETC2_RG11_EAC",        EPixelFormat::PF_ETC2_RG11_EAC },
                { "PF_R8",                   EPixelFormat::PF_R8 },
                { "PF_B5G5R5A1_UNORM",       EPixelFormat::PF_B5G5R5A1_UNORM },
                { "PF_ASTC_4x4_HDR",         EPixelFormat::PF_ASTC_4x4_HDR },
                { "PF_ASTC_6x6_HDR",         EPixelFormat::PF_ASTC_6x6_HDR },
                { "PF_ASTC_8x8_HDR",         EPixelFormat::PF_ASTC_8x8_HDR },
                { "PF_ASTC_10x10_HDR",       EPixelFormat::PF_ASTC_10x10_HDR },
                { "PF_ASTC_12x12_HDR",       EPixelFormat::PF_ASTC_12x12_HDR },
                { "PF_G16R16_SNORM",         EPixelFormat::PF_G16R16_SNORM },
                { "PF_R8G8_UINT",            EPixelFormat::PF_R8G8_UINT },
                { "PF_R32G32B32_UINT",       EPixelFormat::PF_R32G32B32_UINT },
                { "PF_R32G32B32_SINT",       EPixelFormat::PF_R32G32B32_SINT },
                { "PF_R32G32B32F",           EPixelFormat::PF_R32G32B32F },
                { "PF_R8_SINT",              EPixelFormat::PF_R8_SINT },
                { "PF_R64_UINT",             EPixelFormat::PF_R64_UINT },
                { "PF_R9G9B9EXP5",           EPixelFormat::PF_R9G9B9EXP5 },
                { "PF_P010",                 EPixelFormat::PF_P010 },
                { "PF_ASTC_4x4_NORM_RG",     EPixelFormat::PF_ASTC_4x4_NORM_RG },
                { "PF_ASTC_6x6_NORM_RG",     EPixelFormat::PF_ASTC_6x6_NORM_RG },
                { "PF_ASTC_8x8_NORM_RG",     EPixelFormat::PF_ASTC_8x8_NORM_RG },
                { "PF_ASTC_10x10_NORM_RG",   EPixelFormat::PF_ASTC_10x10_NORM_RG },
                { "PF_ASTC_12x12_NORM_RG",   EPixelFormat::PF_ASTC_12x12_NORM_RG },
                { "PF_R16G16_SINT",          EPixelFormat::PF_R16G16_SINT },
                { "PF_R8G8B8",               EPixelFormat::PF_R8G8B8 },
                { "PF_MAX",                  EPixelFormat::PF_MAX },
                { "PF_ASTC_8x5",             EPixelFormat::PF_ASTC_8x5 },
                { "PF_ASTC_8x6",             EPixelFormat::PF_ASTC_8x6 },
                { "PF_ASTC_10x8",            EPixelFormat::PF_ASTC_10x8 },
            };
            return names;
        }

        std::string ToLowerAscii(const std::string& s)
        {
            std::string out;
            out.reserve(s.size());
            for (char c : s) out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            return out;
        }
    }

    namespace PixelFormatUtils
    {
        bool IsHDR(EPixelFormat pixelFormat)
        {
            switch (pixelFormat)
            {
                case EPixelFormat::PF_FloatRGBA:
                case EPixelFormat::PF_BC6H:
                case EPixelFormat::PF_R16F:
                case EPixelFormat::PF_R32_FLOAT:
                case EPixelFormat::PF_A32B32G32R32F:
                case EPixelFormat::PF_ASTC_4x4_HDR:
                case EPixelFormat::PF_ASTC_6x6_HDR:
                case EPixelFormat::PF_ASTC_8x8_HDR:
                case EPixelFormat::PF_ASTC_10x10_HDR:
                case EPixelFormat::PF_ASTC_12x12_HDR:
                    return true;
                default:
                    return false;
            }
        }

        const std::map<EPixelFormat, FPixelFormatInfo>& PixelFormats()
        {
            //       Pixel Format                     Name                     BlockSizeX  BlockSizeY  BlockSizeZ  BlockBytes  NumComponents  Supported by CUE4Parse
            static const std::map<EPixelFormat, FPixelFormatInfo> formats = {
                { EPixelFormat::PF_Unknown,            { EPixelFormat::PF_Unknown,            "unknown",                0,          0,          0,          0,            0,                false } },
                { EPixelFormat::PF_A32B32G32R32F,      { EPixelFormat::PF_A32B32G32R32F,      "A32B32G32R32F",          1,          1,          1,          16,           4,                true  } },
                { EPixelFormat::PF_B8G8R8A8,           { EPixelFormat::PF_B8G8R8A8,           "B8G8R8A8",               1,          1,          1,          4,            4,                true  } },
                { EPixelFormat::PF_G8,                 { EPixelFormat::PF_G8,                 "G8",                     1,          1,          1,          1,            1,                true  } },
                { EPixelFormat::PF_G16,                { EPixelFormat::PF_G16,                "G16",                    1,          1,          1,          2,            1,                true  } },
                { EPixelFormat::PF_DXT1,               { EPixelFormat::PF_DXT1,               "DXT1",                   4,          4,          1,          8,            3,                true  } },
                { EPixelFormat::PF_DXT3,               { EPixelFormat::PF_DXT3,               "DXT3",                   4,          4,          1,          16,           4,                true  } },
                { EPixelFormat::PF_DXT5,               { EPixelFormat::PF_DXT5,               "DXT5",                   4,          4,          1,          16,           4,                true  } },
                { EPixelFormat::PF_UYVY,               { EPixelFormat::PF_UYVY,               "UYVY",                   2,          1,          1,          4,            4,                false } },
                { EPixelFormat::PF_FloatRGB,           { EPixelFormat::PF_FloatRGB,           "FloatRGB",               1,          1,          1,          4,            3,                true  } },
                { EPixelFormat::PF_FloatRGBA,          { EPixelFormat::PF_FloatRGBA,          "FloatRGBA",              1,          1,          1,          8,            4,                true  } },
                { EPixelFormat::PF_DepthStencil,       { EPixelFormat::PF_DepthStencil,       "DepthStencil",           1,          1,          1,          4,            1,                false } },
                { EPixelFormat::PF_ShadowDepth,        { EPixelFormat::PF_ShadowDepth,        "ShadowDepth",            1,          1,          1,          4,            1,                false } },
                { EPixelFormat::PF_R32_FLOAT,          { EPixelFormat::PF_R32_FLOAT,          "R32_FLOAT",              1,          1,          1,          4,            1,                true  } },
                { EPixelFormat::PF_G16R16,             { EPixelFormat::PF_G16R16,             "G16R16",                 1,          1,          1,          4,            2,                true  } },
                { EPixelFormat::PF_G16R16F,            { EPixelFormat::PF_G16R16F,            "G16R16F",                1,          1,          1,          4,            2,                true  } },
                { EPixelFormat::PF_G16R16F_FILTER,     { EPixelFormat::PF_G16R16F_FILTER,     "G16R16F_FILTER",         1,          1,          1,          4,            2,                true  } },
                { EPixelFormat::PF_G32R32F,            { EPixelFormat::PF_G32R32F,            "G32R32F",                1,          1,          1,          8,            2,                true  } },
                { EPixelFormat::PF_A2B10G10R10,        { EPixelFormat::PF_A2B10G10R10,        "A2B10G10R10",            1,          1,          1,          4,            4,                false } },
                { EPixelFormat::PF_A16B16G16R16,       { EPixelFormat::PF_A16B16G16R16,       "A16B16G16R16",           1,          1,          1,          8,            4,                true  } },
                { EPixelFormat::PF_D24,                { EPixelFormat::PF_D24,                "D24",                    1,          1,          1,          4,            1,                false } },
                { EPixelFormat::PF_R16F,               { EPixelFormat::PF_R16F,               "PF_R16F",                1,          1,          1,          2,            1,                true  } },
                { EPixelFormat::PF_R16F_FILTER,        { EPixelFormat::PF_R16F_FILTER,        "PF_R16F_FILTER",         1,          1,          1,          2,            1,                true  } },
                { EPixelFormat::PF_BC5,                { EPixelFormat::PF_BC5,                "BC5",                    4,          4,          1,          16,           2,                true  } },
                { EPixelFormat::PF_V8U8,               { EPixelFormat::PF_V8U8,               "V8U8",                   1,          1,          1,          2,            2,                true  } },
                { EPixelFormat::PF_A1,                 { EPixelFormat::PF_A1,                 "A1",                     1,          1,          1,          1,            1,                false } },
                { EPixelFormat::PF_FloatR11G11B10,     { EPixelFormat::PF_FloatR11G11B10,     "FloatR11G11B10",         1,          1,          1,          4,            3,                false } },
                { EPixelFormat::PF_A8,                 { EPixelFormat::PF_A8,                 "A8",                     1,          1,          1,          1,            1,                false } },
                { EPixelFormat::PF_R32_UINT,           { EPixelFormat::PF_R32_UINT,           "R32_UINT",               1,          1,          1,          4,            1,                false } },
                { EPixelFormat::PF_R32_SINT,           { EPixelFormat::PF_R32_SINT,           "R32_SINT",               1,          1,          1,          4,            1,                false } },

                { EPixelFormat::PF_PVRTC2,             { EPixelFormat::PF_PVRTC2,             "PVRTC2",                 8,          4,          1,          8,            4,                true  } },
                { EPixelFormat::PF_PVRTC4,             { EPixelFormat::PF_PVRTC4,             "PVRTC4",                 4,          4,          1,          8,            4,                true  } },

                { EPixelFormat::PF_R16_UINT,           { EPixelFormat::PF_R16_UINT,           "R16_UINT",               1,          1,          1,          2,            1,                false } },
                { EPixelFormat::PF_R16_SINT,           { EPixelFormat::PF_R16_SINT,           "R16_SINT",               1,          1,          1,          2,            1,                false } },
                { EPixelFormat::PF_R16G16B16A16_UINT,  { EPixelFormat::PF_R16G16B16A16_UINT,  "R16G16B16A16_UINT",      1,          1,          1,          8,            4,                false } },
                { EPixelFormat::PF_R16G16B16A16_SINT,  { EPixelFormat::PF_R16G16B16A16_SINT,  "R16G16B16A16_SINT",      1,          1,          1,          8,            4,                false } },
                { EPixelFormat::PF_R5G6B5_UNORM,       { EPixelFormat::PF_R5G6B5_UNORM,       "PF_R5G6B5_UNORM",        1,          1,          1,          2,            3,                false } },
                { EPixelFormat::PF_R8G8B8A8,           { EPixelFormat::PF_R8G8B8A8,           "R8G8B8A8",               1,          1,          1,          4,            4,                true  } },
                { EPixelFormat::PF_A8R8G8B8,           { EPixelFormat::PF_A8R8G8B8,           "A8R8G8B8",               1,          1,          1,          4,            4,                true  } },
                { EPixelFormat::PF_BC4,                { EPixelFormat::PF_BC4,                "BC4",                    4,          4,          1,          8,            1,                true  } },
                { EPixelFormat::PF_R8G8,               { EPixelFormat::PF_R8G8,               "R8G8",                   1,          1,          1,          2,            2,                false } },

                { EPixelFormat::PF_ATC_RGB,            { EPixelFormat::PF_ATC_RGB,            "ATC_RGB",                4,          4,          1,          8,            3,                false } },
                { EPixelFormat::PF_ATC_RGBA_E,         { EPixelFormat::PF_ATC_RGBA_E,         "ATC_RGBA_E",             4,          4,          1,          16,           4,                false } },
                { EPixelFormat::PF_ATC_RGBA_I,         { EPixelFormat::PF_ATC_RGBA_I,         "ATC_RGBA_I",             4,          4,          1,          16,           4,                false } },
                { EPixelFormat::PF_X24_G8,             { EPixelFormat::PF_X24_G8,             "X24_G8",                 1,          1,          1,          1,            1,                false } },
                { EPixelFormat::PF_ETC1,               { EPixelFormat::PF_ETC1,               "ETC1",                   4,          4,          1,          8,            3,                true  } },
                { EPixelFormat::PF_ETC2_RGB,           { EPixelFormat::PF_ETC2_RGB,           "ETC2_RGB",               4,          4,          1,          8,            3,                true  } },
                { EPixelFormat::PF_ETC2_RGBA,          { EPixelFormat::PF_ETC2_RGBA,          "ETC2_RGBA",              4,          4,          1,          16,           4,                true  } },
                { EPixelFormat::PF_R32G32B32A32_UINT,  { EPixelFormat::PF_R32G32B32A32_UINT,  "PF_R32G32B32A32_UINT",   1,          1,          1,          16,           4,                false } },
                { EPixelFormat::PF_R16G16_UINT,        { EPixelFormat::PF_R16G16_UINT,        "PF_R16G16_UINT",         1,          1,          1,          4,            4,                false } },

                { EPixelFormat::PF_ASTC_4x4,           { EPixelFormat::PF_ASTC_4x4,           "ASTC_4x4",               4,          4,          1,          16,           4,                true  } },
                { EPixelFormat::PF_ASTC_6x6,           { EPixelFormat::PF_ASTC_6x6,           "ASTC_6x6",               6,          6,          1,          16,           4,                true  } },
                { EPixelFormat::PF_ASTC_8x8,           { EPixelFormat::PF_ASTC_8x8,           "ASTC_8x8",               8,          8,          1,          16,           4,                true  } },
                { EPixelFormat::PF_ASTC_10x10,         { EPixelFormat::PF_ASTC_10x10,         "ASTC_10x10",             10,         10,         1,          16,           4,                true  } },
                { EPixelFormat::PF_ASTC_12x12,         { EPixelFormat::PF_ASTC_12x12,         "ASTC_12x12",             12,         12,         1,          16,           4,                true  } },

                { EPixelFormat::PF_BC6H,               { EPixelFormat::PF_BC6H,               "BC6H",                   4,          4,          1,          16,           3,                true  } },
                { EPixelFormat::PF_BC7,                { EPixelFormat::PF_BC7,                "BC7",                    4,          4,          1,          16,           4,                true  } },
                { EPixelFormat::PF_R8_UINT,            { EPixelFormat::PF_R8_UINT,            "R8_UINT",                1,          1,          1,          1,            1,                false } },
                { EPixelFormat::PF_L8,                 { EPixelFormat::PF_L8,                 "L8",                     1,          1,          1,          1,            1,                false } },
                { EPixelFormat::PF_XGXR8,              { EPixelFormat::PF_XGXR8,              "XGXR8",                  1,          1,          1,          4,            4,                false } },
                { EPixelFormat::PF_R8G8B8A8_UINT,      { EPixelFormat::PF_R8G8B8A8_UINT,      "R8G8B8A8_UINT",          1,          1,          1,          4,            4,                false } },
                { EPixelFormat::PF_R8G8B8A8_SNORM,     { EPixelFormat::PF_R8G8B8A8_SNORM,     "R8G8B8A8_SNORM",         1,          1,          1,          4,            4,                false } },

                { EPixelFormat::PF_R16G16B16A16_UNORM, { EPixelFormat::PF_R16G16B16A16_UNORM, "R16G16B16A16_UINT",      1,          1,          1,          8,            4,                false } },
                { EPixelFormat::PF_R16G16B16A16_SNORM, { EPixelFormat::PF_R16G16B16A16_SNORM, "R16G16B16A16_SINT",      1,          1,          1,          8,            4,                false } },
                { EPixelFormat::PF_PLATFORM_HDR_0,     { EPixelFormat::PF_PLATFORM_HDR_0,     "PLATFORM_HDR_0",         0,          0,          0,          0,            0,                false } },
                { EPixelFormat::PF_PLATFORM_HDR_1,     { EPixelFormat::PF_PLATFORM_HDR_1,     "PLATFORM_HDR_1",         0,          0,          0,          0,            0,                false } },
                { EPixelFormat::PF_PLATFORM_HDR_2,     { EPixelFormat::PF_PLATFORM_HDR_2,     "PLATFORM_HDR_2",         0,          0,          0,          0,            0,                false } },

                // NV12 contains 2 textures: R8 luminance plane followed by R8G8 1/4 size chrominance plane.
                // BlockSize/BlockBytes/NumComponents values don't make much sense for this format, so set them all to one.
                { EPixelFormat::PF_NV12,               { EPixelFormat::PF_NV12,               "NV12",                   1,          1,          1,          1,            1,                false } },

                { EPixelFormat::PF_R32G32_UINT,        { EPixelFormat::PF_R32G32_UINT,        "PF_R32G32_UINT",         1,          1,          1,          8,            2,                false } },

                { EPixelFormat::PF_ETC2_R11_EAC,       { EPixelFormat::PF_ETC2_R11_EAC,       "PF_ETC2_R11_EAC",        4,          4,          1,          8,            1,                false } },
                { EPixelFormat::PF_ETC2_RG11_EAC,      { EPixelFormat::PF_ETC2_RG11_EAC,      "PF_ETC2_RG11_EAC",       4,          4,          1,          16,           2,                false } },
                { EPixelFormat::PF_R8,                 { EPixelFormat::PF_R8,                 "R8",                     1,          1,          1,          1,            1,                false } },
                { EPixelFormat::PF_B5G5R5A1_UNORM,     { EPixelFormat::PF_B5G5R5A1_UNORM,     "B5G5R5A1_UNORM",         1,          1,          1,          2,            4,                false } },

                // ASTC HDR support
                { EPixelFormat::PF_ASTC_4x4_HDR,       { EPixelFormat::PF_ASTC_4x4_HDR,       "ASTC_4x4_HDR",           4,          4,          1,          16,           4,                false } },
                { EPixelFormat::PF_ASTC_6x6_HDR,       { EPixelFormat::PF_ASTC_6x6_HDR,       "ASTC_6x6_HDR",           6,          6,          1,          16,           4,                false } },
                { EPixelFormat::PF_ASTC_8x8_HDR,       { EPixelFormat::PF_ASTC_8x8_HDR,       "ASTC_8x8_HDR",           8,          8,          1,          16,           4,                false } },
                { EPixelFormat::PF_ASTC_10x10_HDR,     { EPixelFormat::PF_ASTC_10x10_HDR,     "ASTC_10x10_HDR",         10,         10,         1,          16,           4,                false } },
                { EPixelFormat::PF_ASTC_12x12_HDR,     { EPixelFormat::PF_ASTC_12x12_HDR,     "ASTC_12x12_HDR",         12,         12,         1,          16,           4,                false } },

                { EPixelFormat::PF_G16R16_SNORM,       { EPixelFormat::PF_G16R16_SNORM,       "G16R16_SNORM",           1,          1,          1,          4,            2,                false } },
                { EPixelFormat::PF_R8G8_UINT,          { EPixelFormat::PF_R8G8_UINT,          "R8G8_UINT",              1,          1,          1,          2,            2,                false } },
                { EPixelFormat::PF_R32G32B32_UINT,     { EPixelFormat::PF_R32G32B32_UINT,     "R32G32B32_UINT",         1,          1,          1,          12,           3,                false } },
                { EPixelFormat::PF_R32G32B32_SINT,     { EPixelFormat::PF_R32G32B32_SINT,     "R32G32B32_SINT",         1,          1,          1,          12,           3,                false } },
                { EPixelFormat::PF_R32G32B32F,         { EPixelFormat::PF_R32G32B32F,         "R32G32B32F",             1,          1,          1,          12,           3,                false } },
                { EPixelFormat::PF_R8_SINT,            { EPixelFormat::PF_R8_SINT,            "R8_SINT",                1,          1,          1,          1,            1,                false } },
                { EPixelFormat::PF_R64_UINT,           { EPixelFormat::PF_R64_UINT,           "R64_UINT",               1,          1,          1,          8,            1,                false } },
                { EPixelFormat::PF_R9G9B9EXP5,         { EPixelFormat::PF_R9G9B9EXP5,         "R9G9B9EXP5",             1,          1,          1,          4,            4,                false } },

                // P010 contains 2 textures: R16 luminance plane followed by R16G16 1/4 size chrominance plane. (upper 10 bits used)
                // BlockSize/BlockBytes/NumComponents values don't make much sense for this format, so set them all to one.
                { EPixelFormat::PF_P010,               { EPixelFormat::PF_P010,               "P010",                   1,          1,          1,          2,            1,                false } },

                // ASTC high precision NormalRG support
                { EPixelFormat::PF_ASTC_4x4_NORM_RG,   { EPixelFormat::PF_ASTC_4x4_NORM_RG,   "ASTC_4x4_NORM_RG",       4,          4,          1,          16,           2,                false } },
                { EPixelFormat::PF_ASTC_6x6_NORM_RG,   { EPixelFormat::PF_ASTC_6x6_NORM_RG,   "ASTC_6x6_NORM_RG",       6,          6,          1,          16,           2,                false } },
                { EPixelFormat::PF_ASTC_8x8_NORM_RG,   { EPixelFormat::PF_ASTC_8x8_NORM_RG,   "ASTC_8x8_NORM_RG",       8,          8,          1,          16,           2,                false } },
                { EPixelFormat::PF_ASTC_10x10_NORM_RG, { EPixelFormat::PF_ASTC_10x10_NORM_RG, "ASTC_10x10_NORM_RG",     10,         10,         1,          16,           2,                false } },
                { EPixelFormat::PF_ASTC_12x12_NORM_RG, { EPixelFormat::PF_ASTC_12x12_NORM_RG, "ASTC_12x12_NORM_RG",     12,         12,         1,          16,           2,                false } },

                { EPixelFormat::PF_R16G16_SINT,        { EPixelFormat::PF_R16G16_SINT,        "PF_R16G16_SINT",         1,          1,          1,          4,            4,                false } },
                { EPixelFormat::PF_R8G8B8,             { EPixelFormat::PF_R8G8B8,             "R8G8B8",                 1,          1,          1,          3,            3,                false } },

                // Custom
                { EPixelFormat::PF_ASTC_8x5,           { EPixelFormat::PF_ASTC_8x5,           "PF_ASTC_8x5",            8,          5,          1,          16,           4,                true  } },
                { EPixelFormat::PF_ASTC_8x6,           { EPixelFormat::PF_ASTC_8x6,           "PF_ASTC_8x6",            8,          6,          1,          16,           4,                true  } },
                { EPixelFormat::PF_ASTC_10x8,          { EPixelFormat::PF_ASTC_10x8,          "PF_ASTC_10x8",          10,          8,          1,          16,           4,                true  } },
            };
            return formats;
        }

        const FPixelFormatInfo* TryGetPixelFormatInfo(EPixelFormat pixelFormat)
        {
            const auto& formats = PixelFormats();
            const auto it = formats.find(pixelFormat);
            return it == formats.end() ? nullptr : &it->second;
        }

        bool TryParsePixelFormat(const std::string& text, EPixelFormat& outFormat)
        {
            if (text.empty()) return false;

            static const std::map<std::string, EPixelFormat> byLowerName = []
            {
                std::map<std::string, EPixelFormat> map;
                for (const auto& entry : MemberNames()) map.emplace(ToLowerAscii(entry.first), entry.second);
                return map;
            }();

            const auto it = byLowerName.find(ToLowerAscii(text));
            if (it != byLowerName.end())
            {
                outFormat = it->second;
                return true;
            }

            // Enum.TryParse also accepts the underlying value written out as digits.
            if (text.find_first_not_of("0123456789") == std::string::npos)
            {
                const long value = std::strtol(text.c_str(), nullptr, 10);
                if (value >= 0 && value <= 255)
                {
                    outFormat = static_cast<EPixelFormat>(static_cast<uint8_t>(value));
                    return true;
                }
            }

            return false;
        }
    }
}
