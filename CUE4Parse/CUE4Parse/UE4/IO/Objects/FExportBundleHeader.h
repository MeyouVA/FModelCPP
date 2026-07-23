// Ported from CUE4Parse/UE4/IO/Objects/FExportBundleHeader.cs
// One export bundle: its serial offset (UE5+; MaxValue before) and the entry range it covers.
#pragma once

#include <cstdint>

#include "../../Readers/FArchive.h"
#include "../../Versions/EGame.h"

namespace CUE4Parse::UE4::IO::Objects
{
    struct FExportBundleHeader
    {
        uint64_t SerialOffset = 0;
        uint32_t FirstEntryIndex = 0;
        uint32_t EntryCount = 0;

        FExportBundleHeader() = default;

        explicit FExportBundleHeader(Readers::FArchive& Ar)
        {
            SerialOffset = Ar.Game() >= Versions::GAME_UE5_0 ? Ar.Read<uint64_t>() : UINT64_MAX;
            FirstEntryIndex = Ar.Read<uint32_t>();
            EntryCount = Ar.Read<uint32_t>();
        }
    };
}
