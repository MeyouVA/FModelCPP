// Ported from CUE4Parse/UE4/FMod/Objects/FTimelineNamedMarker.cs
#pragma once

#include <string>

#include "FModGuid.h"
#include "../FModReader.h"

namespace CUE4Parse::UE4::FMod::Objects
{
    struct FTimelineNamedMarker
    {
        FModGuid BaseGuid;
        uint32_t Position = 0;
        std::string Name;
        uint32_t Length = 0;

        FTimelineNamedMarker() = default;
        explicit FTimelineNamedMarker(Readers::FArchive& Ar) : BaseGuid(Ar)
        {
            Position = Ar.Read<uint32_t>();
            Name = FModReader::ReadString(Ar);

            if (FModReader::Version() >= 0x79)
                Length = Ar.Read<uint32_t>();
        }
    };
}
