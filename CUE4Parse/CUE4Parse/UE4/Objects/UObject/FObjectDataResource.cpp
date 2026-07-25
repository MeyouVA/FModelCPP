#include "FObjectDataResource.h"

#include "../../Assets/Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Objects::UObject
{
    FObjectDataResource::FObjectDataResource(Assets::Readers::FAssetArchive& Ar, EObjectDataResourceVersion version)
    {
        Flags = static_cast<EObjectDataResourceFlags>(Ar.Read<uint32_t>());
        if (version >= EObjectDataResourceVersion::AddedCookedIndex)
        {
            CookedIndex = Ar.Read<FBulkDataCookedIndex>();
        }
        SerialOffset = Ar.Read<int64_t>();
        DuplicateSerialOffset = Ar.Read<int64_t>();
        SerialSize = Ar.Read<int64_t>();
        RawSize = Ar.Read<int64_t>();
        OuterIndex = FPackageIndex(Ar);
        LegacyBulkDataFlags = Ar.Read<uint32_t>();
    }
}
