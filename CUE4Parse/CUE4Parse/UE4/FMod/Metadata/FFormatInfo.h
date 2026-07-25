// Ported from CUE4Parse/UE4/FMod/Metadata/FFormatInfo.cs
// The FMT chunk: the bank's file version and a backwards-compat version. FModReader::Version() reads
// FileVersion off the static instance of this struct.
#pragma once

#include "../../Readers/FArchive.h"
#include "../Enums/EFModVersion.h"

namespace CUE4Parse::UE4::FMod::Metadata
{
    struct FFormatInfo
    {
        int32_t FileVersion = 0;
        int32_t CompatVersion = 0;

        FFormatInfo() = default;

        explicit FFormatInfo(Readers::FArchive& Ar)
        {
            FileVersion = Ar.Read<int32_t>();
            // C# warns here when FileVersion > NEWEST_SUPPORTED_FILEVERSION; no logging layer in the port.
            CompatVersion = Ar.Read<int32_t>();
        }
    };
}
