#include "FByteBulkDataHeader.h"

#include "../IoPackage.h"
#include "../Package.h"
#include "../Readers/FAssetArchive.h"
#include "../../Versions/ObjectVersion.h"

namespace CUE4Parse::UE4::Assets::Objects
{
    using CUE4Parse::UE4::Objects::UObject::FObjectDataResource;
    using CUE4Parse::UE4::Versions::EUnrealEngineObjectUE4Version;

    FByteBulkDataHeader::FByteBulkDataHeader(Readers::FAssetArchive& Ar)
    {
        CookedIndex = FBulkDataCookedIndex::Default();

        // UE5 IoPackages carry the bulk-data table in the package header; the "header" on the wire is then
        // just an index into it. A negative or out-of-range index means this is a real inline header after
        // all, so the 4 bytes are given back.
        if (auto* iopkg = dynamic_cast<IoPackage*>(Ar.Owner); iopkg != nullptr && !iopkg->BulkDataMap.empty())
        {
            const int32_t dataIndex = Ar.Read<int32_t>();
            if (dataIndex >= 0 && dataIndex < static_cast<int32_t>(iopkg->BulkDataMap.size()))
            {
                const auto& metaData = iopkg->BulkDataMap[static_cast<size_t>(dataIndex)];
                BulkDataFlags = static_cast<EBulkDataFlags>(metaData.Flags);
                ElementCount = static_cast<int32_t>(metaData.SerialSize);
                SizeOnDisk = static_cast<uint32_t>(metaData.SerialSize); // ??
                OffsetInFile = static_cast<int64_t>(metaData.SerialOffset);
                CookedIndex = metaData.CookedIndex;
                return;
            }
            Ar.Position -= 4;
        }

        // Same shape for a classic package that has a DataResourceMap.
        if (auto* pkg = dynamic_cast<Package*>(Ar.Owner); pkg != nullptr && !pkg->DataResourceMap.empty())
        {
            const int32_t dataIndex = Ar.Read<int32_t>();
            if (dataIndex >= 0 && dataIndex < static_cast<int32_t>(pkg->DataResourceMap.size()))
            {
                const FObjectDataResource& metaData = pkg->DataResourceMap[static_cast<size_t>(dataIndex)];
                BulkDataFlags = static_cast<EBulkDataFlags>(metaData.LegacyBulkDataFlags);
                ElementCount = static_cast<int32_t>(metaData.RawSize);
                OffsetInFile = metaData.SerialOffset;
                SizeOnDisk = static_cast<uint32_t>(metaData.SerialSize);
                CookedIndex = metaData.CookedIndex;
                return;
            }
            Ar.Position -= 4;
        }

        BulkDataFlags = Ar.Read<EBulkDataFlags>();
        ElementCount = HasFlag(BulkDataFlags, EBulkDataFlags::BULKDATA_Size64Bit)
            ? static_cast<int32_t>(Ar.Read<int64_t>()) : Ar.Read<int32_t>();
        SizeOnDisk = HasFlag(BulkDataFlags, EBulkDataFlags::BULKDATA_Size64Bit)
            ? static_cast<uint32_t>(Ar.Read<int64_t>()) : Ar.Read<uint32_t>();
        OffsetInFile = Ar.Ver() >= EUnrealEngineObjectUE4Version::BULKDATA_AT_LARGE_OFFSETS
            ? Ar.Read<int64_t>() : static_cast<int64_t>(Ar.Read<int32_t>());
        if (!HasFlag(BulkDataFlags, EBulkDataFlags::BULKDATA_NoOffsetFixUp)) // UE4.26 flag
        {
            OffsetInFile += Ar.Owner->GetSummary()->BulkDataStartOffset;
        }

        if (HasFlag(BulkDataFlags, EBulkDataFlags::BULKDATA_BadDataVersion))
        {
            Ar.Position += sizeof(uint16_t);
            BulkDataFlags &= ~EBulkDataFlags::BULKDATA_BadDataVersion;
        }

        if (HasFlag(BulkDataFlags, EBulkDataFlags::BULKDATA_DuplicateNonOptionalPayload))
        {
            Ar.Position += sizeof(EBulkDataFlags); // DuplicateFlags
            Ar.Position += HasFlag(BulkDataFlags, EBulkDataFlags::BULKDATA_Size64Bit)
                ? sizeof(int64_t) : sizeof(uint32_t);  // DuplicateSizeOnDisk
            Ar.Position += Ar.Ver() >= EUnrealEngineObjectUE4Version::BULKDATA_AT_LARGE_OFFSETS
                ? sizeof(int64_t) : sizeof(int32_t);   // DuplicateOffset
        }
    }
}
