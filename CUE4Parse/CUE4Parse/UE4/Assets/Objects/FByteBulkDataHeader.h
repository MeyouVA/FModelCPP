// Ported from CUE4Parse/UE4/Assets/Objects/FByteBulkDataHeader.cs
// Where a bulk payload lives: which file (via the flags), at what offset, how big. Read three different
// ways depending on the owning package -- out of a UE5 IoPackage's BulkDataMap, out of a classic Package's
// DataResourceMap, or inline off the archive -- and the reading constructor tries them in that order,
// rewinding the 4-byte index read when the map lookup misses.
//
// Deliberate differences from C#:
//   * The reading constructor lives in the .cpp: it needs the concrete Package/IoPackage types, which
//     include FAssetArchive, which includes this header. The value-only constructors stay inline so
//     GameFile and the VFS readers can take a `const FByteBulkDataHeader*` on a forward declaration.
//   * C# is a readonly struct; here the fields are plain and the type is copyable, which is all the call
//     sites need. No JSON converter (the port has no JSON layer).
#pragma once

#include <cstdint>

#include "EBulkDataFlags.h"
#include "../../IO/Objects/FBulkDataMapEntry.h"

namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }

namespace CUE4Parse::UE4::Assets::Objects
{
    using CUE4Parse::UE4::IO::Objects::FBulkDataCookedIndex;

    struct FByteBulkDataHeader
    {
        EBulkDataFlags BulkDataFlags = EBulkDataFlags::BULKDATA_None;
        int32_t ElementCount = 0;
        uint32_t SizeOnDisk = 0;
        int64_t OffsetInFile = 0;
        FBulkDataCookedIndex CookedIndex;

        FByteBulkDataHeader() = default;

        FByteBulkDataHeader(EBulkDataFlags bulkDataFlags, int32_t elementCount, uint32_t sizeOnDisk,
                            int64_t offsetInFile, FBulkDataCookedIndex cookedIndex)
            : BulkDataFlags(bulkDataFlags), ElementCount(elementCount), SizeOnDisk(sizeOnDisk),
              OffsetInFile(offsetInFile), CookedIndex(cookedIndex) {}

        // C#'s copy constructor; the implicit one would do, but the call sites spell it out.
        FByteBulkDataHeader(const FByteBulkDataHeader& header) = default;
        FByteBulkDataHeader& operator=(const FByteBulkDataHeader& header) = default;

        explicit FByteBulkDataHeader(Readers::FAssetArchive& Ar);
    };
}
