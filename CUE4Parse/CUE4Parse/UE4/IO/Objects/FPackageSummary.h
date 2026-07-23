// Ported from CUE4Parse/UE4/IO/Objects/FPackageSummary.cs
// The UE4 (4.25-4.27) Zen package summary — fixed-layout, read as one block.
#pragma once

#include <cstdint>

#include "FMappedName.h"
#include "../../Objects/UObject/EPackageFlags.h"

namespace CUE4Parse::UE4::IO::Objects
{
    struct FPackageSummary
    {
        FMappedName Name;
        FMappedName SourceName;
        uint32_t PackageFlags = 0; // EPackageFlags
        uint32_t CookedHeaderSize = 0;
        int32_t NameMapNamesOffset = 0;
        int32_t NameMapNamesSize = 0;
        int32_t NameMapHashesOffset = 0;
        int32_t NameMapHashesSize = 0;
        int32_t ImportMapOffset = 0;
        int32_t ExportMapOffset = 0;
        int32_t ExportBundlesOffset = 0;
        int32_t GraphDataOffset = 0;
        int32_t GraphDataSize = 0;
        int32_t _pad = 0;
    };
    static_assert(sizeof(FPackageSummary) == 64);
}
