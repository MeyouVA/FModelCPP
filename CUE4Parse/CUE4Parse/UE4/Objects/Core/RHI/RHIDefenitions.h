// Ported from CUE4Parse/UE4/Objects/Core/RHI/RHIDefenitions.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Objects::Core::RHI
{
    enum class EPrimitiveType
    {
        PT_TriangleList,
        PT_TriangleStrip,
        PT_LineList,
        PT_QuadList,
        PT_PointList,
        PT_RectList,
        PT_1_ControlPointPatchList,
        PT_2_ControlPointPatchList,
        PT_3_ControlPointPatchList,
        PT_4_ControlPointPatchList,
        PT_5_ControlPointPatchList,
        PT_6_ControlPointPatchList,
        PT_7_ControlPointPatchList,
        PT_8_ControlPointPatchList,
        PT_9_ControlPointPatchList,
        PT_10_ControlPointPatchList,
        PT_11_ControlPointPatchList,
        PT_12_ControlPointPatchList,
        PT_13_ControlPointPatchList,
        PT_14_ControlPointPatchList,
        PT_15_ControlPointPatchList,
        PT_16_ControlPointPatchList,
        PT_17_ControlPointPatchList,
        PT_18_ControlPointPatchList,
        PT_19_ControlPointPatchList,
        PT_20_ControlPointPatchList,
        PT_21_ControlPointPatchList,
        PT_22_ControlPointPatchList,
        PT_23_ControlPointPatchList,
        PT_24_ControlPointPatchList,
        PT_25_ControlPointPatchList,
        PT_26_ControlPointPatchList,
        PT_27_ControlPointPatchList,
        PT_28_ControlPointPatchList,
        PT_29_ControlPointPatchList,
        PT_30_ControlPointPatchList,
        PT_31_ControlPointPatchList,
        PT_32_ControlPointPatchList,
        PT_Num,
        PT_NumBits                  = 6,
    };

    enum class ERenderTargetLoadAction : uint8_t
    {
        ENoAction,
        ELoad,
        EClear,
        Num,
        NumBits   = 2,
    };

    enum class ERenderTargetStoreAction : uint8_t
    {
        ENoAction,
        EStore,
        EMultisampleResolve,
        Num,
        NumBits             = 2,
    };

    enum class ETextureCreateFlags : uint64_t
    {
        None                                = 0,
        RenderTargetable                    = 1u << 0,
        ResolveTargetable                   = 1u << 1,
        DepthStencilTargetable              = 1u << 2,
        ShaderResource                      = 1u << 3,
        SRGB                                = 1u << 4,
        CPUWritable                         = 1u << 5,
        NoTiling                            = 1u << 6,
        VideoDecode                         = 1u << 7,
        Dynamic                             = 1u << 8,
        InputAttachmentRead                 = 1u << 9,
        Foveation                           = 1u << 10,
        Tiling3D                            = 1u << 11,
        Memoryless                          = 1u << 12,
        //UE_DEPRECATED(5.5,"The GenerateMipCapable flag is no longer used.")
        GenerateMipCapable                  = 1u << 13,
        FastVRAMPartialAlloc                = 1u << 14,
        DisableSRVCreation                  = 1u << 15,
        DisableDCC                          = 1u << 16,
        UAV                                 = 1u << 17,
        Presentable                         = 1u << 18,
        CPUReadback                         = 1u << 19,
        OfflineProcessed                    = 1u << 20,
        FastVRAM                            = 1u << 21,
        HideInVisualizeTexture              = 1u << 22,
        Virtual                             = 1u << 23,
        TargetArraySlicesIndependently      = 1u << 24,
        Shared                              = 1u << 25,
        NoFastClear                         = 1u << 26,
        DepthStencilResolveTarget           = 1u << 27,
        Streamable                          = 1u << 28,
        NoFastClearFinalize                 = 1u << 29,
        AFRManual                           = 1u << 30,
        ReduceMemoryWithTilingMode          = 1u << 31,
        //UE_DEPRECATED(5.0, "ETextureCreateFlags::Transient flag is no longer used.")
        Transient                           = None,
        AtomicCompatible                    = 1u << 33,
        External                            = 1u << 34,
        MultiGPUGraphIgnore                 = 1u << 35,
        ReservedResource                    = 1u << 37,
        ImmediateCommit                     = 1u << 38,
        ForceIntoNonStreamingMemoryTracking = 1u << 39,
        Invalid                             = 1u << 40,
    };

    enum class EVertexElementType : uint8_t
    {
        VET_None,
        VET_Float1,
        VET_Float2,
        VET_Float3,
        VET_Float4,
        VET_PackedNormal,
        VET_UByte4,
        VET_UByte4N,
        VET_Color,
        VET_Short2,
        VET_Short4,
        VET_Short2N,
        VET_Half2,
        VET_Half4,
        VET_Short4N,
        VET_UShort2,
        VET_UShort4,
        VET_UShort2N,
        VET_UShort4N,
        VET_URGB10A2N,
        VET_UInt,
        VET_MAX,
        VET_NumBits      = 5,
    };

    enum class ERHIZBuffer
    {
        FarPlane   = 0,
        NearPlane  = 1,
        IsInverted = 0, // (int)(FarPlane < NearPlane),
    };

    enum class ECompareFunction : uint8_t
    {
        CF_Less,
        CF_LessEqual,
        CF_Greater,
        CF_GreaterEqual,
        CF_Equal,
        CF_NotEqual,
        CF_Never,
        CF_Always,
        ECompareFunction_Num,
        ECompareFunction_NumBits = 3,
        CF_DepthNearOrEqual      = static_cast<int32_t>(ERHIZBuffer::IsInverted) != 0 ? CF_GreaterEqual : CF_LessEqual,
        CF_DepthNear             = static_cast<int32_t>(ERHIZBuffer::IsInverted) != 0 ? CF_Greater : CF_Less,
        CF_DepthFartherOrEqual   = static_cast<int32_t>(ERHIZBuffer::IsInverted) != 0 ? CF_LessEqual : CF_GreaterEqual,
        CF_DepthFarther          = static_cast<int32_t>(ERHIZBuffer::IsInverted) != 0 ? CF_Less : CF_Greater,
    };

    enum class ERasterizerFillMode
    {
        FM_Point,
        FM_Wireframe,
        FM_Solid,
        ERasterizerFillMode_Num,
        ERasterizerFillMode_NumBits = 2,
    };

    enum class ERasterizerCullMode
    {
        CM_None,
        CM_CW,
        CM_CCW,
        ERasterizerCullMode_Num,
        ERasterizerCullMode_NumBits = 2,
    };

    enum class ERasterizerDepthClipMode : uint8_t
    {
        DepthClip,
        DepthClamp,
        Num,
        NumBits    = 1,
    };

    enum class EStencilMask
    {
        SM_Default,
        SM_255,
        SM_1,
        SM_2,
        SM_4,
        SM_8,
        SM_16,
        SM_32,
        SM_64,
        SM_128,
        SM_Count,
    };

    enum class EStencilOp : uint8_t
    {
        SO_Keep,
        SO_Zero,
        SO_Replace,
        SO_SaturatedIncrement,
        SO_SaturatedDecrement,
        SO_Invert,
        SO_Increment,
        SO_Decrement,
        EStencilOp_Num,
        EStencilOp_NumBits    = 3,
    };

    enum class EBlendOperation : uint8_t
    {
        BO_Add,
        BO_Subtract,
        BO_Min,
        BO_Max,
        BO_ReverseSubtract,
        EBlendOperation_Num,
        EBlendOperation_NumBits = 3,
    };

    enum class EBlendFactor : uint8_t
    {
        BF_Zero,
        BF_One,
        BF_SourceColor,
        BF_InverseSourceColor,
        BF_SourceAlpha,
        BF_InverseSourceAlpha,
        BF_DestAlpha,
        BF_InverseDestAlpha,
        BF_DestColor,
        BF_InverseDestColor,
        BF_ConstantBlendFactor,
        BF_InverseConstantBlendFactor,
        BF_Source1Color,
        BF_InverseSource1Color,
        BF_Source1Alpha,
        BF_InverseSource1Alpha,
        EBlendFactor_Num,
        EBlendFactor_NumBits          = 4,
    };

    enum class EColorWriteMask : uint8_t
    {
        CW_RED                  = 0x01,
        CW_GREEN                = 0x02,
        CW_BLUE                 = 0x04,
        CW_ALPHA                = 0x08,
        CW_NONE                 = 0,
        CW_RGB                  = CW_RED | CW_GREEN | CW_BLUE,
        CW_RGBA                 = CW_RED | CW_GREEN | CW_BLUE | CW_ALPHA,
        CW_RG                   = CW_RED | CW_GREEN,
        CW_BA                   = CW_BLUE | CW_ALPHA,
        EColorWriteMask_NumBits = 4,
    };
}
