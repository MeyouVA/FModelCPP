// Ported from CUE4Parse/UE4/Assets/Objects/EBulkDataFlags.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Assets::Objects
{
    // C# [Flags] enum: the bitwise operators C# gets for free are spelled out below.
    enum class EBulkDataFlags : uint32_t
    {
        BULKDATA_None                            = 0,
        BULKDATA_PayloadAtEndOfFile              = 1u << 0,
        BULKDATA_SerializeCompressedZLIB         = 1u << 1,
        BULKDATA_ForceSingleElementSerialization = 1u << 2,
        BULKDATA_SingleUse                       = 1u << 3,
        BULKDATA_CompressedLZO                   = 1u << 4,
        BULKDATA_Unused                          = 1u << 5,
        BULKDATA_ForceInlinePayload              = 1u << 6,
        BULKDATA_SerializeCompressed             = BULKDATA_SerializeCompressedZLIB,
        BULKDATA_ForceStreamPayload              = 1u << 7,
        BULKDATA_PayloadInSeperateFile           = 1u << 8,
        BULKDATA_SerializeCompressedBitWindow    = 1u << 9,
        BULKDATA_Force_NOT_InlinePayload         = 1u << 10,
        BULKDATA_OptionalPayload                 = 1u << 11,
        BULKDATA_MemoryMappedPayload             = 1u << 12,
        BULKDATA_Size64Bit                       = 1u << 13,
        BULKDATA_DuplicateNonOptionalPayload     = 1u << 14,
        BULKDATA_BadDataVersion                  = 1u << 15,
        BULKDATA_NoOffsetFixUp                   = 1u << 16,
        BULKDATA_WorkspaceDomainPayload          = 1u << 17,
        BULKDATA_LazyLoadable                    = 1u << 18,
        // BULKDATA_UsesIoDispatcher                   = 1u << 31,
        BULKDATA_DataIsMemoryMapped              = 1u << 30,
        BULKDATA_HasAsyncReadPending             = 1u << 29,
        BULKDATA_AlwaysAllowDiscard              = 1u << 28,
    };

    constexpr EBulkDataFlags operator|(EBulkDataFlags a, EBulkDataFlags b)
    { return static_cast<EBulkDataFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b)); }
    constexpr EBulkDataFlags operator&(EBulkDataFlags a, EBulkDataFlags b)
    { return static_cast<EBulkDataFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b)); }
    constexpr EBulkDataFlags operator~(EBulkDataFlags a)
    { return static_cast<EBulkDataFlags>(static_cast<uint32_t>(~static_cast<uint32_t>(a))); }
    constexpr EBulkDataFlags& operator|=(EBulkDataFlags& a, EBulkDataFlags b) { a = a | b; return a; }
    constexpr EBulkDataFlags& operator&=(EBulkDataFlags& a, EBulkDataFlags b) { a = a & b; return a; }
    // C#'s Enum.HasFlag.
    constexpr bool HasFlag(EBulkDataFlags value, EBulkDataFlags flag)
    { return (static_cast<uint32_t>(value) & static_cast<uint32_t>(flag)) == static_cast<uint32_t>(flag); }
}
