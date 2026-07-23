// Ported from CUE4Parse/UE4/IO/Objects/FIoContainerHeader.cs
// The container header chunk: the package-id -> store-entry tables (plus optional-segment tables), the
// redirect/localized-package tables and their name map. This is what LoadPackage consults to find a Zen
// package's imported package ids.
#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "FFilePackageStoreEntry.h"
#include "FIoContainerId.h"
#include "FMappedName.h"
#include "FPackageId.h"
#include "FIoContainerHeaderSerialInfo.h"
#include "../../Objects/UObject/FNameEntrySerialized.h"
#include "../../Readers/FArchive.h"

namespace CUE4Parse::UE4::IO::Objects
{
    using CUE4Parse::UE4::Objects::UObject::FNameEntrySerialized;

    struct FIoContainerHeaderLocalizedPackage
    {
        FPackageId SourcePackageId;
        FMappedName SourcePackageName;
    };
    static_assert(sizeof(FIoContainerHeaderLocalizedPackage) == 16);

    struct FIoContainerHeaderPackageRedirect
    {
        FPackageId SourcePackageId;
        FPackageId TargetPackageId;
        FMappedName SourcePackageName;
    };
    static_assert(sizeof(FIoContainerHeaderPackageRedirect) == 24);

    struct FIoContainerHeaderSoftPackageReferences
    {
        std::vector<FPackageId> PackageIds;
        std::vector<uint8_t> PackageIndices;
        bool bContainsSoftPackageReferences = false;

        FIoContainerHeaderSoftPackageReferences() = default;
        explicit FIoContainerHeaderSoftPackageReferences(Readers::FArchive& Ar);
    };

    class FIoContainerHeader
    {
    public:
        static constexpr int32_t Signature = 0x496f436e;

        EIoContainerHeaderVersion Version = EIoContainerHeaderVersion::BeforeVersionWasAdded;
        FIoContainerId ContainerId;

        std::vector<FPackageId> PackageIds;
        std::vector<FFilePackageStoreEntry> StoreEntries;
        std::vector<FFilePackageStoreEntry> OptionalSegmentStoreEntries;
        std::vector<FPackageId> OptionalSegmentPackageIds;

        std::optional<std::vector<FNameEntrySerialized>> ContainerNameMap; // RedirectsNameMap
        std::vector<FIoContainerHeaderLocalizedPackage> LocalizedPackages;
        std::vector<FIoContainerHeaderPackageRedirect> PackageRedirects;
        FIoContainerHeaderSerialInfo SoftPackageReferencesSerialInfo;
        FIoContainerHeaderSoftPackageReferences SoftPackageReferences;

        explicit FIoContainerHeader(Readers::FArchive& Ar);

    private:
        void ReadPackageIdsAndEntries(Readers::FArchive& Ar, std::vector<FPackageId>& packageIds,
                                      std::vector<FFilePackageStoreEntry>& storeEntries);
    };
}
