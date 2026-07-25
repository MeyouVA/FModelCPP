// Ported from CUE4Parse/UE4/FMod/Metadata/SoundDataInfo.cs
// The SNDH chunk: an element list of (FSBOffset, Length) headers, one per SND chunk. Held by unique_ptr
// on FModReader's static SoundDataInfo (C#'s nullable class field).
#pragma once

#include <vector>

#include "../FModReader.h"

namespace CUE4Parse::UE4::FMod::Metadata
{
    class SoundDataInfo
    {
    public:
        struct FSoundDataHeader
        {
            uint32_t FSBOffset = 0;
            uint32_t Length = 0;

            FSoundDataHeader() = default;
            explicit FSoundDataHeader(Readers::FArchive& Ar)
            {
                FSBOffset = Ar.Read<uint32_t>();
                Length = Ar.Read<uint32_t>();
            }
        };

        std::vector<FSoundDataHeader> Header;

        explicit SoundDataInfo(Readers::FArchive& Ar)
        {
            Header = FModReader::ReadElemListImp<FSoundDataHeader>(Ar);
        }
    };
}
