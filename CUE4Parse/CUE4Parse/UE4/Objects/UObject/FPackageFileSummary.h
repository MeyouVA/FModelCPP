// Ported from CUE4Parse/UE4/Objects/UObject/FPackageFileSummary.cs
// Revision/header data at the top of every .uasset package. Owns the PACKAGE_FILE_TAG constants.
//
// Deliberate differences from C#:
//   * C#'s Log.Warning for out-of-range file versions is dropped (no logging framework here);
//     the condition is noted in a comment at its site.
//   * The Tag == PACKAGE_FILE_TAG_ONE / FAssetArchive.AbsoluteOffset fix-up is deferred until the
//     asset-reader layer (FAssetArchive) is ported. TODO at its site.
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "EPackageFlags.h"
#include "../Core/Misc/FGuid.h"
#include "../Core/Misc/FSHAHash.h"
#include "../Core/Misc/FEngineVersion.h"
#include "../Core/Misc/ECompressionFlags.h"
#include "../Core/Serialization/FCustomVersionContainer.h"
#include "../../Versions/FPackageFileVersion.h"
#include "../../Versions/ObjectVersion.h"

namespace CUE4Parse::UE4::Readers { class FArchive; }

namespace CUE4Parse::UE4::Objects::UObject
{
    using Versions::FPackageFileVersion;
    using Versions::EUnrealEngineObjectLicenseeUEVersion;
    using Core::Misc::FGuid;
    using Core::Misc::FSHAHash;
    using Core::Misc::FEngineVersion;
    using Core::Misc::ECompressionFlags;
    using Core::Serialization::FCustomVersionContainer;

    // Revision data for an Unreal package file (a generation of the linker's export/name maps).
    struct FGenerationInfo
    {
        int32_t ExportCount = 0; // Number of exports in the linker's ExportMap for this generation.
        int32_t NameCount = 0;   // Number of names in the linker's NameMap for this generation.
    };

    class FPackageFileSummary
    {
    public:
        static constexpr uint32_t PACKAGE_FILE_TAG = 0x9E2A83C1U;
        static constexpr uint32_t PACKAGE_FILE_TAG_SWAPPED = 0xC1832A9EU;
        static constexpr uint32_t PACKAGE_FILE_TAG_ACE7 = 0x37454341U; // ACE7
        static constexpr uint32_t PACKAGE_FILE_TAG_ONE = 0x00656E6FU;  // SOD2
        static constexpr uint32_t PACKAGE_FILE_TAG_AE = 0x56DE5ECAU;   // AshEchoes

        uint32_t Tag = 0;
        FPackageFileVersion FileVersionUE;
        EUnrealEngineObjectLicenseeUEVersion FileVersionLicenseeUE = EUnrealEngineObjectLicenseeUEVersion::LIC_NONE;
        FCustomVersionContainer CustomVersionContainer;
        EPackageFlags PackageFlags = PKG_None;
        int32_t TotalHeaderSize = 0;
        std::string PackageName;
        int32_t NameCount = 0;
        int32_t NameOffset = 0;
        int32_t SoftObjectPathsCount = 0;
        int32_t SoftObjectPathsOffset = 0;
        std::string LocalizationId;
        int32_t GatherableTextDataCount = 0;
        int32_t GatherableTextDataOffset = 0;
        int32_t MetaDataOffset = 0;
        int32_t ExportCount = 0;
        int32_t ExportOffset = 0;
        int32_t ImportCount = 0;
        int32_t ImportOffset = 0;
        int32_t CellExportCount = 0;
        int32_t CellExportOffset = 0;
        int32_t CellImportCount = 0;
        int32_t CellImportOffset = 0;
        int32_t DependsOffset = 0;
        int32_t SoftPackageReferencesCount = 0;
        int32_t SoftPackageReferencesOffset = 0;
        int32_t SearchableNamesOffset = 0;
        int32_t ThumbnailTableOffset = 0;
        int32_t ImportTypeHierarchiesCount = 0;
        int32_t ImportTypeHierarchiesOffset = 0;
        FSHAHash SavedHash;
        FGuid Guid;
        FGuid PersistentGuid;
        std::vector<FGenerationInfo> Generations;
        std::optional<FEngineVersion> SavedByEngineVersion;
        std::optional<FEngineVersion> CompatibleWithEngineVersion;
        ECompressionFlags CompressionFlags = Core::Misc::COMPRESS_None;
        int32_t PackageSource = 0;
        bool bUnversioned = false;
        int32_t AssetRegistryDataOffset = 0;
        int32_t BulkDataStartOffset = 0; // serialized as long
        int32_t WorldTileInfoDataOffset = 0;
        std::vector<int32_t> ChunkIds;
        int32_t PreloadDependencyCount = 0;
        int32_t PreloadDependencyOffset = 0;
        int32_t NamesReferencedFromExportDataCount = 0;
        int64_t PayloadTocOffset = 0;
        int32_t DataResourceOffset = 0;

        FPackageFileSummary() = default;
        explicit FPackageFileSummary(Readers::FArchive& Ar);

    private:
        static void FixCorruptEngineVersion(const FPackageFileVersion& objectVersion, FEngineVersion& version);
    };
}
