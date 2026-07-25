// Ported from CUE4Parse/UE4/Wwise/Objects/MediaHeader.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Objects
{
    // AkBank::MediaHeader
    // Blitted straight out of the BankDataIndex section (C# marks it [StructLayout(Sequential)] and reads
    // sectionLength / 12 of them), so the 12-byte layout is load-bearing.
#pragma pack(push, 1)
    struct MediaHeader
    {
        uint32_t Id;
        uint32_t Offset;
        int32_t Size;
    };
#pragma pack(pop)
    static_assert(sizeof(MediaHeader) == 12, "MediaHeader is read as a 12-byte blit from BankDataIndex");
}
