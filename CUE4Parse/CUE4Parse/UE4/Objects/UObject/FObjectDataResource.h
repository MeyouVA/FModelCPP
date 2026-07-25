// Ported from CUE4Parse/UE4/Objects/UObject/FObjectDataResource.cs
// One entry of a classic package's DataResourceMap: the UE5 replacement for the inline bulk-data header,
// describing where an export's bulk payload lives. FByteBulkDataHeader reads out of this table when the
// owning Package has one.
#pragma once

#include <cstdint>

#include "ObjectResource.h"
#include "../../IO/Objects/FBulkDataMapEntry.h"

namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }

namespace CUE4Parse::UE4::Objects::UObject
{
    using CUE4Parse::UE4::IO::Objects::FBulkDataCookedIndex;

    enum class EObjectDataResourceFlags : uint32_t
    {
        None = 0,
        Inline = (1 << 0),
        Streaming = (1 << 1),
        Optional = (1 << 2),
        Duplicate = (1 << 3),
        MemoryMapped = (1 << 4),
        DerivedDataReference = (1 << 5),
    };

    enum class EObjectDataResourceVersion : uint32_t
    {
        Invalid,
        Initial,
        AddedCookedIndex,

        LatestPlusOne,
        Latest = LatestPlusOne - 1
    };

    struct FObjectDataResource
    {
        EObjectDataResourceFlags Flags = EObjectDataResourceFlags::None;
        FBulkDataCookedIndex CookedIndex;
        int64_t SerialOffset = -1;
        int64_t DuplicateSerialOffset = -1;
        int64_t SerialSize = -1;
        int64_t RawSize = -1;
        FPackageIndex OuterIndex;
        uint32_t LegacyBulkDataFlags = 0;

        FObjectDataResource() = default;
        FObjectDataResource(Assets::Readers::FAssetArchive& Ar, EObjectDataResourceVersion version);
    };
}
