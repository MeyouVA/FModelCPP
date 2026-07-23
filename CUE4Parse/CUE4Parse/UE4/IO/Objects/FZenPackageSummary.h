// Ported from CUE4Parse/UE4/IO/Objects/FZenPackageSummary.cs
// The UE5 Zen package summary (+ the optional versioning-info block and the 5.5 Verse cell offsets).
#pragma once

#include <cstdint>

#include "FMappedName.h"
#include "../../Objects/Core/Serialization/FCustomVersionContainer.h"
#include "../../Objects/UObject/EPackageFlags.h"
#include "../../Readers/FArchive.h"
#include "../../Versions/EGame.h"
#include "../../Versions/FPackageFileVersion.h"

namespace CUE4Parse::UE4::IO::Objects
{
    using CUE4Parse::UE4::Objects::Core::Serialization::FCustomVersionContainer;
    using CUE4Parse::UE4::Versions::FPackageFileVersion;

    enum class EZenPackageVersion : uint32_t
    {
        Initial,
        DataResourceTable,
        ImportedPackageNames,
        ExportDependencies,

        LatestPlusOne,
        Latest = LatestPlusOne - 1
    };

    struct FZenPackageVersioningInfo
    {
        EZenPackageVersion ZenVersion = EZenPackageVersion::Initial;
        FPackageFileVersion PackageVersion;
        int32_t LicenseeVersion = 0;
        FCustomVersionContainer CustomVersions;

        explicit FZenPackageVersioningInfo(Readers::FArchive& Ar);
    };

    struct FZenPackageCellOffsets
    {
        int32_t CellImportMapOffset = 0;
        int32_t CellExportMapOffset = 0;
    };

    struct FZenPackageSummary
    {
        uint32_t bHasVersioningInfo = 0;
        uint32_t HeaderSize = 0;
        FMappedName Name;
        CUE4Parse::UE4::Objects::UObject::EPackageFlags PackageFlags{};
        uint32_t CookedHeaderSize = 0;
        int32_t ImportedPublicExportHashesOffset = 0;
        int32_t ImportMapOffset = 0;
        int32_t ExportMapOffset = 0;
        int32_t ExportBundleEntriesOffset = 0;
        int32_t GraphDataOffset = 0;
        int32_t DependencyBundleHeadersOffset = 0;
        int32_t DependencyBundleEntriesOffset = 0;
        int32_t ImportedPackageNamesOffset = 0;

        FZenPackageSummary() = default;
        explicit FZenPackageSummary(Readers::FArchive& Ar);
    };
}
