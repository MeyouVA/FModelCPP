// Ported from CUE4Parse/UE4/Objects/UObject/FPackageFileSummary.cs
#include "FPackageFileSummary.h"

#include <cstdio>
#include <memory>

#include "../../Readers/FArchive.h"
#include "../../Versions/EGame.h"
#include "../../Versions/VersionContainer.h"
#include "../../Assets/Objects/FCompressedChunk.h"
#include "../../Exceptions/ParserException.h"

namespace CUE4Parse::UE4::Objects::UObject
{
    using namespace CUE4Parse::UE4::Versions;
    using namespace CUE4Parse::UE4::Objects::Core::Misc; // ECompressionFlags enumerators (COMPRESS_*)
    using CUE4Parse::UE4::Assets::Objects::FCompressedChunk;
    using CUE4Parse::UE4::Objects::Core::Serialization::FCustomVersionContainer;
    namespace Exceptions = CUE4Parse::UE4::Exceptions;

    FPackageFileSummary::FPackageFileSummary(Readers::FArchive& Ar)
    {
        Tag = Ar.Read<uint32_t>();

        // See the C# source for the full history of the legacy file version sentinels (-2 .. -9).
        const int32_t CurrentLegacyFileVersion = -9;
        int32_t legacyFileVersion = CurrentLegacyFileVersion;

        if (Tag == PACKAGE_FILE_TAG_ONE) // SOD2, "one"
        {
            Ar.SetGame(GAME_StateOfDecay2);
            Ar.SetVer(GetVersion(Ar.Game()));
            legacyFileVersion = Ar.Read<int32_t>(); // seems to be always int.MinValue
            bUnversioned = true;
            FileVersionUE = Ar.Ver();
            CustomVersionContainer = FCustomVersionContainer();
            PackageName = "None";
            PackageFlags = PKG_FilterEditorOnly;
            // fall through to the "afterPackageFlags" reads below
        }
        else
        {
            if (Tag == PACKAGE_FILE_TAG_AE) Tag = PACKAGE_FILE_TAG;

            if (Tag != PACKAGE_FILE_TAG && Tag != PACKAGE_FILE_TAG_SWAPPED)
            {
                char buf[128];
                std::snprintf(buf, sizeof(buf), "Invalid uasset magic: 0x%08X != 0x%08X", Tag, PACKAGE_FILE_TAG);
                throw Exceptions::ParserException(buf);
            }

            legacyFileVersion = Ar.Read<int32_t>();
            if (Ar.Game() == GAME_DeltaForce) legacyFileVersion /= 659;

            if (legacyFileVersion < 0) // means we have modern version numbers
            {
                if (legacyFileVersion < CurrentLegacyFileVersion)
                {
                    // We can't safely load more than this; make sure the linker fails to load with it.
                    FileVersionUE.Reset();
                    FileVersionLicenseeUE = EUnrealEngineObjectLicenseeUEVersion::LIC_NONE;
                    throw Exceptions::ParserException("Can't load legacy UE3 file");
                }

                if (legacyFileVersion != -4)
                {
                    FileVersionUE.FileVersionUE3 = Ar.Read<int32_t>();
                }

                FileVersionUE.FileVersionUE4 = Ar.Read<int32_t>();
                if (Ar.Game() == GAME_DaysGone) FileVersionUE.FileVersionUE4 = 498;

                if (legacyFileVersion <= -8)
                {
                    FileVersionUE.FileVersionUE5 = Ar.Read<int32_t>();
                }

                FileVersionLicenseeUE = Ar.Read<EUnrealEngineObjectLicenseeUEVersion>();

                // C# logs a warning here when the file version is out of the loadable range; no-op in the port.

                if (FileVersionUE.FileVersionUE4 == 0 && FileVersionUE.FileVersionUE5 == 0 &&
                    static_cast<int32_t>(FileVersionLicenseeUE) == 0)
                {
                    // this file is unversioned, remember that, then use current versions
                    bUnversioned = true;
                    FileVersionUE = Ar.Ver();
                    FileVersionLicenseeUE = EUnrealEngineObjectLicenseeUEVersion::LIC_AUTOMATIC_VERSION;
                }
                else
                {
                    bUnversioned = false;
                    // Only apply the version if an explicit version is not set
                    if (!Ar.Versions.bExplicitVer)
                    {
                        Ar.SetVer(FileVersionUE);
                    }
                }

                if ((Ar.Versions.bExplicitVer ? Ar.Ver() : FileVersionUE) >= EUnrealEngineObjectUE5Version::PACKAGE_SAVED_HASH)
                {
                    SavedHash = FSHAHash(Ar);
                    TotalHeaderSize = Ar.Read<int32_t>();
                }

                CustomVersionContainer = FCustomVersionContainer(
                    Ar, FCustomVersionContainer::DetermineSerializationFormat(legacyFileVersion));

                if (!Ar.Versions.CustomVersions && !CustomVersionContainer.Versions.empty())
                {
                    Ar.Versions.CustomVersions = std::make_shared<FCustomVersionContainer>(CustomVersionContainer);
                }
            }
            else
            {
                // This is probably an old UE3 file, make sure that the linker will fail to load with it.
                throw Exceptions::ParserException("Can't load legacy UE3 file");
            }

            if (FileVersionUE < EUnrealEngineObjectUE5Version::PACKAGE_SAVED_HASH)
            {
                TotalHeaderSize = Ar.Read<int32_t>();
            }

            PackageName = Ar.ReadFString(); // PackageGroup
            PackageFlags = Ar.Read<EPackageFlags>();
        }

        // afterPackageFlags:
        NameCount = Ar.Read<int32_t>();
        NameOffset = Ar.Read<int32_t>();

        if (FileVersionUE >= EUnrealEngineObjectUE5Version::ADD_SOFTOBJECTPATH_LIST)
        {
            SoftObjectPathsCount = Ar.Read<int32_t>();
            SoftObjectPathsOffset = Ar.Read<int32_t>();
        }

        if ((PackageFlags & PKG_FilterEditorOnly) == 0)
        {
            if (FileVersionUE >= EUnrealEngineObjectUE4Version::ADDED_PACKAGE_SUMMARY_LOCALIZATION_ID)
            {
                LocalizationId = Ar.ReadFString();
            }
        }

        if (FileVersionUE >= EUnrealEngineObjectUE4Version::SERIALIZE_TEXT_IN_PACKAGES)
        {
            GatherableTextDataCount = Ar.Read<int32_t>();
            GatherableTextDataOffset = Ar.Read<int32_t>();
        }

        ExportCount = Ar.Read<int32_t>();
        ExportOffset = Ar.Read<int32_t>();
        ImportCount = Ar.Read<int32_t>();
        ImportOffset = Ar.Read<int32_t>();

        if (FileVersionUE >= EUnrealEngineObjectUE5Version::VERSE_CELLS)
        {
            CellExportCount = Ar.Read<int32_t>();
            CellExportOffset = Ar.Read<int32_t>();
            CellImportCount = Ar.Read<int32_t>();
            CellImportOffset = Ar.Read<int32_t>();
        }

        if (FileVersionUE >= EUnrealEngineObjectUE5Version::METADATA_SERIALIZATION_OFFSET)
        {
            MetaDataOffset = Ar.Read<int32_t>();
        }

        DependsOffset = Ar.Read<int32_t>();

        if (FileVersionUE < EUnrealEngineObjectUE4Version::OLDEST_LOADABLE_PACKAGE ||
            FileVersionUE > EUnrealEngineObjectUE4Version::AUTOMATIC_VERSION)
        {
            Generations.clear();
            ChunkIds.clear();
            return; // can't safely load more than this: the below differed in older files.
        }

        if (FileVersionUE >= EUnrealEngineObjectUE4Version::ADD_STRING_ASSET_REFERENCES_MAP)
        {
            SoftPackageReferencesCount = Ar.Read<int32_t>();
            SoftPackageReferencesOffset = Ar.Read<int32_t>();
        }

        if (FileVersionUE >= EUnrealEngineObjectUE4Version::ADDED_SEARCHABLE_NAMES)
        {
            SearchableNamesOffset = Ar.Read<int32_t>();
        }

        ThumbnailTableOffset = Ar.Read<int32_t>();

        if (FileVersionUE >= EUnrealEngineObjectUE5Version::IMPORT_TYPE_HIERARCHIES || Ar.Game() == GAME_DeltaForce)
        {
            ImportTypeHierarchiesCount = Ar.Read<int32_t>();
            ImportTypeHierarchiesOffset = Ar.Read<int32_t>();
        }
        else
        {
            ImportTypeHierarchiesCount = 0;
            ImportTypeHierarchiesOffset = 0;
        }

        if (FileVersionUE < EUnrealEngineObjectUE5Version::PACKAGE_SAVED_HASH)
        {
            Guid = Ar.Read<FGuid>();
        }

        if (Ar.Game() == GAME_Valorant_PRE_11_2 || Ar.Game() == GAME_HYENAS) Ar.Position += 8;

        if ((PackageFlags & PKG_FilterEditorOnly) == 0)
        {
            if (FileVersionUE >= EUnrealEngineObjectUE4Version::ADDED_PACKAGE_OWNER)
            {
                PersistentGuid = Ar.Read<FGuid>();
            }
            else
            {
                // Assign the current package guid so we keep a stable persistent guid even if not resaved.
                PersistentGuid = Guid;
            }

            // The owner persistent guid was added in ADDED_PACKAGE_OWNER but removed in NON_OUTER_PACKAGE_IMPORT.
            if (FileVersionUE >= EUnrealEngineObjectUE4Version::ADDED_PACKAGE_OWNER &&
                FileVersionUE < EUnrealEngineObjectUE4Version::NON_OUTER_PACKAGE_IMPORT)
            {
                (void) Ar.Read<FGuid>(); // ownerPersistentGuid (discarded)
            }
        }

        Generations = Ar.ReadArrayCounted<FGenerationInfo>();

        if (FileVersionUE >= EUnrealEngineObjectUE4Version::ENGINE_VERSION_OBJECT)
        {
            SavedByEngineVersion.emplace(Ar);
            FixCorruptEngineVersion(FileVersionUE, *SavedByEngineVersion);
        }
        else
        {
            const int32_t engineChangelist = Ar.Read<int32_t>();
            if (engineChangelist != 0)
            {
                SavedByEngineVersion.emplace(4, 0, 0, static_cast<uint32_t>(engineChangelist), std::string());
            }
        }

        if (FileVersionUE >= EUnrealEngineObjectUE4Version::PACKAGE_SUMMARY_HAS_COMPATIBLE_ENGINE_VERSION)
        {
            CompatibleWithEngineVersion.emplace(Ar);
            FixCorruptEngineVersion(FileVersionUE, *CompatibleWithEngineVersion);
        }
        else
        {
            CompatibleWithEngineVersion = SavedByEngineVersion;
        }

        CompressionFlags = Ar.Read<ECompressionFlags>();

        const int32_t CompressionFlagsMask =
            COMPRESS_DeprecatedFormatFlagsMask | COMPRESS_OptionsFlagsMask | COMPRESS_ForPurposeMask;
        if ((static_cast<int32_t>(CompressionFlags) & ~CompressionFlagsMask) != 0)
        {
            throw Exceptions::ParserException(
                "Invalid compression flags (" + std::to_string(static_cast<uint32_t>(CompressionFlags)) + ")");
        }

        auto compressedChunks = Ar.ReadArrayCounted<FCompressedChunk>();
        if (!compressedChunks.empty())
        {
            throw Exceptions::ParserException("Package level compression is enabled");
        }

        PackageSource = Ar.Read<int32_t>();

        if (Ar.Game() == GAME_ArkSurvivalEvolved && static_cast<int32_t>(FileVersionLicenseeUE) >= 10)
        {
            Ar.Position += 8;
        }

        // No longer used: list of additional packages needed to be cooked for this package (streaming levels).
        (void) Ar.ReadArrayWith([&Ar]() { return Ar.ReadFString(); });

        if (legacyFileVersion > -7)
        {
            const int32_t numTextureAllocations = Ar.Read<int32_t>();
            if (numTextureAllocations != 0)
            {
                throw Exceptions::ParserException("NumTextureAllocations != 0");
            }
        }

        if (FileVersionUE >= EUnrealEngineObjectUE4Version::ASSET_REGISTRY_TAGS)
        {
            AssetRegistryDataOffset = Ar.Read<int32_t>();
        }

        if (Ar.Game() == GAME_TowerOfFantasy)
        {
            auto x = [](int32_t v) { return static_cast<int32_t>(static_cast<uint32_t>(v) ^ 0xEEB2CEC7u); };
            TotalHeaderSize = x(TotalHeaderSize);
            NameCount = x(NameCount);
            NameOffset = x(NameOffset);
            ExportCount = x(ExportCount);
            ExportOffset = x(ExportOffset);
            ImportCount = x(ImportCount);
            ImportOffset = x(ImportOffset);
            DependsOffset = x(DependsOffset);
            PackageSource = x(PackageSource);
            AssetRegistryDataOffset = x(AssetRegistryDataOffset);
        }

        if (Ar.Game() == GAME_SeaOfThieves || Ar.Game() == GAME_GearsOfWar4)
        {
            Ar.Position += 6; // no idea what's going on here.
        }

        if (FileVersionUE >= EUnrealEngineObjectUE4Version::SUMMARY_HAS_BULKDATA_OFFSET)
        {
            BulkDataStartOffset = static_cast<int32_t>(Ar.Read<int64_t>());
        }

        if (FileVersionUE >= EUnrealEngineObjectUE4Version::WORLD_LEVEL_INFO)
        {
            WorldTileInfoDataOffset = Ar.Read<int32_t>();
        }

        if (FileVersionUE >= EUnrealEngineObjectUE4Version::CHANGED_CHUNKID_TO_BE_AN_ARRAY_OF_CHUNKIDS)
        {
            ChunkIds = Ar.ReadArrayCounted<int32_t>();
        }
        else if (FileVersionUE >= EUnrealEngineObjectUE4Version::ADDED_CHUNKID_TO_ASSETDATA_AND_UPACKAGE)
        {
            const int32_t chunkId = Ar.Read<int32_t>();
            ChunkIds.clear();
            if (chunkId >= 0) ChunkIds.push_back(chunkId);
        }
        else
        {
            ChunkIds.clear();
        }

        if (FileVersionUE >= EUnrealEngineObjectUE4Version::PRELOAD_DEPENDENCIES_IN_COOKED_EXPORTS)
        {
            PreloadDependencyCount = Ar.Read<int32_t>();
            PreloadDependencyOffset = Ar.Read<int32_t>();
        }
        else
        {
            PreloadDependencyCount = -1;
            PreloadDependencyOffset = 0;
        }

        NamesReferencedFromExportDataCount =
            FileVersionUE >= EUnrealEngineObjectUE5Version::NAMES_REFERENCED_FROM_EXPORT_DATA ? Ar.Read<int32_t>() : NameCount;
        PayloadTocOffset =
            FileVersionUE >= EUnrealEngineObjectUE5Version::PAYLOAD_TOC ? Ar.Read<int64_t>() : -1;
        DataResourceOffset =
            (FileVersionUE >= EUnrealEngineObjectUE5Version::DATA_RESOURCES || Ar.Game() == GAME_TheFirstDescendant)
                ? Ar.Read<int32_t>() : -1;

        // TODO: the Tag == PACKAGE_FILE_TAG_ONE / FAssetArchive.AbsoluteOffset fix-up arrives with the
        // asset-reader layer (FAssetArchive is not ported yet).
    }

    void FPackageFileSummary::FixCorruptEngineVersion(const FPackageFileVersion& objectVersion, FEngineVersion& version)
    {
        if (objectVersion < EUnrealEngineObjectUE4Version::CORRECT_LICENSEE_FLAG &&
            version.Major == 4 && version.Minor == 26 && version.Patch == 0 && version.Changelist() >= 12740027 &&
            version.IsLicenseeVersion())
        {
            version.Set(4, 26, 0, version.Changelist(), version.Branch());
        }
    }
}
