// Ported from CUE4Parse/UE4/Assets/IoPackage.cs
#include "IoPackage.h"

#include <algorithm>
#include <exception>

#include "ObjectTypeRegistry.h"
#include "../Exceptions/ParserException.h"
#include "../IO/Objects/FExportBundleEntry.h"
#include "../IO/Objects/FExportBundleHeader.h"
#include "../IO/Objects/FPackageSummary.h"
#include "../IO/Objects/FZenPackageSummary.h"
#include "../Objects/Core/Serialization/FCustomVersionContainer.h"
#include "../Objects/UObject/UScriptClass.h"
#include "../Versions/EGame.h"
#include "../Versions/ObjectVersion.h"
#include "../../FileProvider/Vfs/IVfsFileProvider.h"

namespace CUE4Parse::UE4::Assets
{
    using namespace CUE4Parse::UE4::Versions;
    using CUE4Parse::UE4::Exceptions::MappingException;
    using CUE4Parse::UE4::Exceptions::ParserException;
    using CUE4Parse::UE4::Objects::Core::Serialization::FCustomVersionContainer;
    using CUE4Parse::UE4::Objects::UObject::PKG_ContainsNoAsset;
    using CUE4Parse::UE4::Objects::UObject::PKG_UnversionedProperties;
    using Readers::ESeekOrigin;

    using IO::Objects::EExportCommandType;
    using IO::Objects::FBulkDataMapEntry;
    using IO::Objects::FExportBundleEntry;
    using IO::Objects::FExportBundleHeader;
    using IO::Objects::FExportMapEntry;
    using IO::Objects::FFilePackageStoreEntry;
    using IO::Objects::FMappedName;
    using IO::Objects::FPackageId;
    using IO::Objects::FPackageObjectIndex;
    using IO::Objects::FPackageSummary;
    using IO::Objects::FZenPackageCellOffsets;
    using IO::Objects::FZenPackageSummary;
    using IO::Objects::FZenPackageVersioningInfo;

    namespace
    {
        int IndexOf(const std::vector<FPackageId>& ids, const FPackageId& id)
        {
            for (size_t i = 0; i < ids.size(); i++)
                if (ids[i] == id) return static_cast<int>(i);
            return -1;
        }
    }

    // A resolved entry from this package's ExportMap.
    class IoPackage::ResolvedExportObject : public ResolvedObject
    {
    public:
        ResolvedExportObject(CUE4Parse::UE4::Assets::IoPackage* package, int exportIndex)
            : ResolvedObject(package, exportIndex), _export(&package->ExportMap[static_cast<size_t>(exportIndex)]) {}

        FName Name() const override { return Pkg()->CreateFNameFromMappedName(_export->ObjectName); }
        ResolvedObject* Outer() const override
        {
            ResolvedObject* outer = Pkg()->ResolveObjectIndex(_export->OuterIndex);
            return outer != nullptr ? outer : Pkg()->GetSelfResolved();
        }
        ResolvedObject* Class() const override { return Pkg()->ResolveObjectIndex(_export->ClassIndex); }
        ResolvedObject* Super() const override { return Pkg()->ResolveObjectIndex(_export->SuperIndex); }

    private:
        // `this->Package` is the inherited IPackage* member; the enclosing type name would shadow it.
        CUE4Parse::UE4::Assets::IoPackage* Pkg() const
        {
            return static_cast<CUE4Parse::UE4::Assets::IoPackage*>(this->Package);
        }

        const FExportMapEntry* _export;
    };

    // A resolved global script object (a /Script/... class, struct or function).
    class IoPackage::ResolvedScriptObject : public ResolvedObject
    {
    public:
        ResolvedScriptObject(CUE4Parse::UE4::Assets::IoPackage* package, IO::Objects::FScriptObjectEntry scriptImport)
            : ResolvedObject(package), ScriptImport(scriptImport) {}

        IO::Objects::FScriptObjectEntry ScriptImport;

        FName Name() const override { return Pkg()->CreateFNameFromMappedName(ScriptImport.ObjectName); }
        ResolvedObject* Outer() const override { return Pkg()->ResolveObjectIndex(ScriptImport.OuterIndex); }
        // C#: Object => new UScriptClass(Name.Text). Unversioned deserialization needs this — it resolves the
        // export's class to a UStruct to pick the mappings schema — so it IS ported (cached, owned here).
        Exports::UObject* Object() const override
        {
            if (!_object)
                _object = std::make_unique<CUE4Parse::UE4::Objects::UObject::UScriptClass>(Name().Text());
            return _object.get();
        }
        // C#'s Class => new ResolvedLoadedObject(new UScriptClass("Class")) stays null: ResolvedLoadedObject
        // is not ported (see ResolvedObject.h). Nothing in this port reads a script object's own class.

    private:
        mutable std::unique_ptr<CUE4Parse::UE4::Objects::UObject::UScriptClass> _object;

        CUE4Parse::UE4::Assets::IoPackage* Pkg() const
        {
            return static_cast<CUE4Parse::UE4::Assets::IoPackage*>(this->Package);
        }
    };

    IoPackage::IoPackage(FArchive& uasset, IO::Objects::FIoContainerHeader* containerHeader,
                         Readers::FAssetArchive::RawPayloadProvider ubulk,
                         Readers::FAssetArchive::RawPayloadProvider uptnl,
                         CUE4Parse::FileProvider::Vfs::IVfsFileProvider* provider)
        : _provider(provider)
    {
        _name = uasset.Name();
        if (const auto dot = _name.find_last_of('.'); dot != std::string::npos) _name = _name.substr(0, dot);

        _globalData = provider != nullptr ? provider->GlobalData() : nullptr;
        if (_globalData == nullptr)
            throw ParserException("Found IoStore Package but global data is missing, can't serialize");

        _uassetAr = std::make_unique<FAssetArchive>(uasset, this);
        FAssetArchive& uassetAr = *_uassetAr;

        std::vector<FExportBundleHeader> exportBundleHeaders;
        bool hasExportBundleHeaders = false;
        std::vector<FExportBundleEntry> exportBundleEntries;

        if (uassetAr.Game() >= GAME_UE5_0)
        {
            // Summary
            const FZenPackageSummary summary(uassetAr);
            Summary = FPackageFileSummary();
            Summary.PackageFlags = summary.PackageFlags;
            Summary.TotalHeaderSize = summary.GraphDataOffset + static_cast<int>(summary.HeaderSize);
            Summary.NameOffset = static_cast<int>(uassetAr.Position);
            Summary.ExportCount = (summary.ExportBundleEntriesOffset - summary.ExportMapOffset) / FExportMapEntry::Size;
            Summary.ExportOffset = summary.ExportMapOffset;
            Summary.ImportCount = (summary.ExportMapOffset - summary.ImportMapOffset) / FPackageObjectIndex::Size;
            Summary.ImportOffset = summary.ImportMapOffset;

            // Versioning info
            if (summary.bHasVersioningInfo != 0)
            {
                const FZenPackageVersioningInfo versioningInfo(uassetAr);
                Summary.FileVersionUE = versioningInfo.PackageVersion;
                Summary.FileVersionLicenseeUE = static_cast<EUnrealEngineObjectLicenseeUEVersion>(versioningInfo.LicenseeVersion);
                Summary.CustomVersionContainer = versioningInfo.CustomVersions;
                if (!uassetAr.Versions.bExplicitVer)
                {
                    uassetAr.Versions.SetVer(versioningInfo.PackageVersion);
                    uassetAr.Versions.CustomVersions = std::make_shared<FCustomVersionContainer>(versioningInfo.CustomVersions);
                }
            }
            else
            {
                Summary.bUnversioned = true;
            }

            FZenPackageCellOffsets cellOffsets;
            if (uassetAr.Ver() >= EUnrealEngineObjectUE5Version::VERSE_CELLS)
            {
                cellOffsets = uassetAr.Read<FZenPackageCellOffsets>();
            }
            else
            {
                cellOffsets.CellImportMapOffset = summary.ExportBundleEntriesOffset;
                cellOffsets.CellExportMapOffset = summary.ExportBundleEntriesOffset;
            }

            // Name map
            NameMapEntries = FNameEntrySerialized::LoadNameBatch(uassetAr);
            Summary.NameCount = static_cast<int>(NameMapEntries.size());
            _name = CreateFNameFromMappedName(summary.Name).Text();

            BulkDataMap.clear();
            if (uassetAr.Ver() >= EUnrealEngineObjectUE5Version::DATA_RESOURCES ||
                uassetAr.Game() == GAME_TheFirstDescendant)
            {
                if (uassetAr.Game() >= GAME_UE5_4)
                {
                    const auto pad = uassetAr.Read<uint64_t>();
                    uassetAr.Position += static_cast<int64_t>(pad); // C# reads and discards `pad` bytes
                }

                const auto bulkDataMapSize = uassetAr.Read<int64_t>();
                BulkDataMap = uassetAr.ReadArray<FBulkDataMapEntry>(
                    static_cast<int>(bulkDataMapSize / FBulkDataMapEntry::Size));
            }

            // Imported public export hashes
            uassetAr.Position = summary.ImportedPublicExportHashesOffset;
            ImportedPublicExportHashes = uassetAr.ReadArray<uint64_t>(
                (summary.ImportMapOffset - summary.ImportedPublicExportHashesOffset) / static_cast<int>(sizeof(uint64_t)));
            _hasPublicExportHashes = true; // C#: the array is non-null only on this (UE5) path

            // Import map
            uassetAr.Position = summary.ImportMapOffset;
            ImportMap = uassetAr.ReadArray<FPackageObjectIndex>(Summary.ImportCount);

            // Export map
            uassetAr.Position = summary.ExportMapOffset;
            ExportMap.clear();
            ExportMap.reserve(static_cast<size_t>(Summary.ExportCount));
            for (int i = 0; i < Summary.ExportCount; i++) ExportMap.emplace_back(uassetAr);
            ExportsLazy.resize(static_cast<size_t>(Summary.ExportCount));
            _exportLoadInfo.resize(static_cast<size_t>(Summary.ExportCount));
            _exportResolved.resize(static_cast<size_t>(Summary.ExportCount));

            // Export bundle entries
            uassetAr.Position = cellOffsets.CellImportMapOffset;
            exportBundleEntries = uassetAr.ReadArray<FExportBundleEntry>(Summary.ExportCount * 2);

            const FFilePackageStoreEntry* storeEntry = nullptr;
            GetStoreEntryAndImportedPackageIds(containerHeader, storeEntry, _importedPackageIds);
            if (uassetAr.Game() < GAME_UE5_3)
            {
                // Export bundle headers
                uassetAr.Position = summary.GraphDataOffset;
                const int exportBundleHeadersCount = storeEntry != nullptr ? storeEntry->ExportBundleCount : 1;
                exportBundleHeaders = uassetAr.ReadArray<FExportBundleHeader>(exportBundleHeadersCount);
                hasExportBundleHeaders = true;
                // We don't read the graph data
            }

            _cookedHeaderSize = static_cast<int>(summary.CookedHeaderSize);
            _allExportDataOffset = static_cast<int>(summary.HeaderSize);
        }
        else
        {
            // Summary
            const auto summary = uassetAr.Read<FPackageSummary>();
            Summary = FPackageFileSummary();
            Summary.PackageFlags = static_cast<CUE4Parse::UE4::Objects::UObject::EPackageFlags>(summary.PackageFlags);
            Summary.TotalHeaderSize = summary.GraphDataOffset + summary.GraphDataSize;
            Summary.NameCount = summary.NameMapHashesSize / static_cast<int>(sizeof(uint64_t)) - 1;
            Summary.NameOffset = summary.NameMapNamesOffset;
            Summary.ExportCount = (summary.ExportBundlesOffset - summary.ExportMapOffset) / FExportMapEntry::Size;
            Summary.ExportOffset = summary.ExportMapOffset;
            Summary.ImportCount = (summary.ExportMapOffset - summary.ImportMapOffset) / FPackageObjectIndex::Size;
            Summary.ImportOffset = summary.ImportMapOffset;
            Summary.bUnversioned = true;

            // Name map
            uassetAr.Position = summary.NameMapNamesOffset;
            NameMapEntries = FNameEntrySerialized::LoadNameBatch(uassetAr, Summary.NameCount);
            _name = CreateFNameFromMappedName(summary.Name).Text();

            // Import map
            uassetAr.Position = summary.ImportMapOffset;
            ImportMap = uassetAr.ReadArray<FPackageObjectIndex>(Summary.ImportCount);

            // Export map
            uassetAr.Position = summary.ExportMapOffset;
            ExportMap.clear();
            ExportMap.reserve(static_cast<size_t>(Summary.ExportCount));
            for (int i = 0; i < Summary.ExportCount; i++) ExportMap.emplace_back(uassetAr);
            ExportsLazy.resize(static_cast<size_t>(Summary.ExportCount));
            _exportLoadInfo.resize(static_cast<size_t>(Summary.ExportCount));
            _exportResolved.resize(static_cast<size_t>(Summary.ExportCount));

            // Export bundles
            uassetAr.Position = summary.ExportBundlesOffset;
            LoadExportBundles(uassetAr, summary.GraphDataOffset - summary.ExportBundlesOffset,
                              exportBundleHeaders, exportBundleEntries);
            hasExportBundleHeaders = true;

            // Graph data
            uassetAr.Position = summary.GraphDataOffset;
            _importedPackageIds = LoadGraphData(uassetAr);

            _cookedHeaderSize = static_cast<int>(summary.CookedHeaderSize);
            _allExportDataOffset = summary.GraphDataOffset + summary.GraphDataSize;
        }

        // Preload dependencies stay lazy: ImportedPackages() resolves _importedPackageIds on first use.

        if (!CanDeserialize()) return;

        // Attach ubulk and uptnl
        if (ubulk != nullptr)
            uassetAr.AddPayload(Utils::PayloadType::UBULK, Summary.BulkDataStartOffset, std::move(ubulk));
        if (uptnl != nullptr)
            uassetAr.AddPayload(Utils::PayloadType::UPTNL, Summary.BulkDataStartOffset, std::move(uptnl));

        // Record where each export's serial data starts; GetExportObject deserializes from there on demand.
        const auto processEntry = [this](const FExportBundleEntry& entry, int pos, bool newPos) -> int
        {
            if (entry.CommandType != EExportCommandType::ExportCommandType_Serialize)
                return 0; // Skip ExportCommandType_Create

            if (entry.LocalExportIndex >= ExportMap.size())
                throw ParserException("Export bundle entry references export " +
                                      std::to_string(entry.LocalExportIndex) + " outside the export map");

            const FExportMapEntry& exp = ExportMap[entry.LocalExportIndex];
            _exportLoadInfo[entry.LocalExportIndex] = ExportLoadInfo{true, pos, newPos};
            return static_cast<int>(exp.CookedSerialSize);
        };

        if (hasExportBundleHeaders) // 4.26 - 5.2
        {
            int currentExportDataOffset = _allExportDataOffset;
            for (const auto& exportBundle : exportBundleHeaders)
            {
                for (uint32_t i = 0; i < exportBundle.EntryCount; i++)
                {
                    const size_t entryIndex = static_cast<size_t>(exportBundle.FirstEntryIndex) + i;
                    if (entryIndex >= exportBundleEntries.size())
                        throw ParserException("Export bundle header references entries outside the bundle-entry array");
                    currentExportDataOffset += processEntry(exportBundleEntries[entryIndex], currentExportDataOffset, false);
                }
                Summary.BulkDataStartOffset = currentExportDataOffset;
            }
        }
        else for (const auto& entry : exportBundleEntries)
        {
            if (entry.LocalExportIndex >= ExportMap.size())
                throw ParserException("Export bundle entry references export outside the export map");
            processEntry(entry, _allExportDataOffset + static_cast<int>(ExportMap[entry.LocalExportIndex].CookedSerialOffset), true);
        }

        IsFullyLoaded = true;
    }

    void IoPackage::GetStoreEntryAndImportedPackageIds(IO::Objects::FIoContainerHeader* containerHeader,
                                                       const FFilePackageStoreEntry*& outStoreEntry,
                                                       std::vector<FPackageId>& outImportedPackageIds)
    {
        // Find store entry by package name
        outStoreEntry = nullptr;
        const FFilePackageStoreEntry* mainAssetStoreEntry = nullptr;
        outImportedPackageIds.clear();
        if (containerHeader != nullptr)
        {
            const FPackageId packageId = FPackageId::FromName(_name);
            const int storeEntryIdx = IndexOf(containerHeader->PackageIds, packageId);
            if (storeEntryIdx != -1 && storeEntryIdx < static_cast<int>(containerHeader->StoreEntries.size()))
            {
                outStoreEntry = &containerHeader->StoreEntries[static_cast<size_t>(storeEntryIdx)];
            }
            else
            {
                const int optionalSegmentStoreEntryIdx = IndexOf(containerHeader->OptionalSegmentPackageIds, packageId);
                if (optionalSegmentStoreEntryIdx != -1 &&
                    optionalSegmentStoreEntryIdx < static_cast<int>(containerHeader->OptionalSegmentStoreEntries.size()))
                {
                    outStoreEntry = &containerHeader->OptionalSegmentStoreEntries[static_cast<size_t>(optionalSegmentStoreEntryIdx)];
                }
                else
                {
                    // this should not happen for regular packages, but can be the case for editor only data
                    // (C# logs a warning when it is still not found; no logging layer here)
                    mainAssetStoreEntry = _provider != nullptr ? _provider->TryFindStoreEntry(packageId) : nullptr;
                }
            }
        }

        // C#'s `storeEntry?.ImportedPackages is null`: an empty vector stands in for the null array.
        if ((outStoreEntry == nullptr || outStoreEntry->ImportedPackages.empty()) &&
            mainAssetStoreEntry != nullptr && !mainAssetStoreEntry->ImportedPackages.empty() &&
            HasFlags(PKG_ContainsNoAsset))
        {
            if (ExportMap.empty() || !ExportMap[0].OuterIndex.IsPackageImport())
                return;

            // manually inserting main package as outer
            const int index = static_cast<int>(ExportMap[0].OuterIndex.AsPackageImportRef().ImportedPackageIndex);
            const std::vector<FPackageId>& list = mainAssetStoreEntry->ImportedPackages;
            const int listLen = static_cast<int>(list.size());

            const int minv = index < listLen ? index : listLen;
            const int maxv = index < listLen ? listLen : index;
            if (maxv > 1024 * 1024)
            {
                outImportedPackageIds = list;
                return;
            }

            std::vector<FPackageId> result(static_cast<size_t>(maxv) + 1);
            std::copy(list.begin(), list.begin() + minv, result.begin());
            result[static_cast<size_t>(index)] = FPackageId::FromName(_name);
            if (index < listLen)
                std::copy(list.begin() + index, list.end(), result.begin() + index + 1);

            outImportedPackageIds = std::move(result);
        }
        else
        {
            outImportedPackageIds = outStoreEntry != nullptr ? outStoreEntry->ImportedPackages : std::vector<FPackageId>{};
        }
    }

    void IoPackage::LoadExportBundles(FArchive& Ar, int graphDataSize,
                                      std::vector<FExportBundleHeader>& bundleHeaders,
                                      std::vector<FExportBundleEntry>& bundleEntries)
    {
        int remainingBundleEntryCount = graphDataSize / (4 + 4);
        int foundBundlesCount = 0;
        bundleHeaders.clear();
        while (foundBundlesCount < remainingBundleEntryCount)
        {
            // This location is occupied by header, so it is not a bundle entry
            remainingBundleEntryCount--;
            FExportBundleHeader bundleHeader(Ar);
            foundBundlesCount += static_cast<int>(bundleHeader.EntryCount);
            bundleHeaders.push_back(bundleHeader);
        }

        if (foundBundlesCount != remainingBundleEntryCount)
            throw ParserException(Ar, "FoundBundlesCount " + std::to_string(foundBundlesCount) +
                                     " != RemainingBundleEntryCount " + std::to_string(remainingBundleEntryCount));

        bundleEntries = Ar.ReadArray<FExportBundleEntry>(foundBundlesCount);
    }

    std::vector<FPackageId> IoPackage::LoadGraphData(FArchive& Ar)
    {
        if (Ar.Game() == GAME_NeedForSpeedMobile && Ar.ReadBoolean()) Ar.Position += 8;
        const auto packageCount = Ar.Read<int32_t>();
        if (packageCount == 0) return {};

        std::vector<FPackageId> packageIds(static_cast<size_t>(packageCount));
        for (int packageIndex = 0; packageIndex < packageCount; packageIndex++)
        {
            const auto packageId = Ar.Read<FPackageId>();
            const auto bundleCount = Ar.Read<int32_t>();
            Ar.Position += static_cast<int64_t>(bundleCount) * (sizeof(int32_t) + sizeof(int32_t)); // Skip FArcs
            packageIds[static_cast<size_t>(packageIndex)] = packageId;
        }

        return packageIds;
    }

    bool IoPackage::CanDeserialize() const
    {
        if (HasFlags(PKG_UnversionedProperties) && Mappings() == nullptr)
            throw MappingException("Package has unversioned properties but mapping file is missing, can't serialize");
        return true;
    }

    std::unique_ptr<Exports::UObject> IoPackage::ConstructObject(ResolvedObject* struc)
    {
        // Same simplification as Package::ConstructObject (see the long comment there): the registry is keyed
        // on the resolved class object's name instead of walking the UStruct/UClass chain.
        std::unique_ptr<Exports::UObject> obj;
        if (struc != nullptr)
        {
            if (auto factory = ObjectTypeRegistry::Get(struc->Name().Text()))
                obj = factory();
        }
        if (!obj)
            obj = std::make_unique<Exports::UObject>();
        obj->Class = struc;
        obj->Flags = static_cast<Exports::EObjectFlags>(obj->Flags | Exports::RF_WasLoaded);
        return obj;
    }

    void IoPackage::DeserializeObject(Exports::UObject& obj, FAssetArchive& Ar, int64_t serialSize)
    {
        if (serialSize == 0) return; // Empty export.
        const int64_t validPos = Ar.Position + serialSize;
        try
        {
            obj.Deserialize(Ar, validPos);
        }
        catch (const std::exception&)
        {
            // As in Package::DeserializeObject: C# logs here, this port has no logging layer.
        }
    }

    Exports::UObject* IoPackage::GetExportObject(int index)
    {
        if (index < 0 || index >= static_cast<int>(ExportsLazy.size())) return nullptr;
        const size_t i = static_cast<size_t>(index);
        if (!ExportsLazy[i])
        {
            // C# leaves the Lazy null for an export no bundle issued a Serialize command for; touching it
            // there would throw, here it simply has nothing to load.
            const ExportLoadInfo info = _exportLoadInfo[i];
            if (!info.Valid) return nullptr;

            const FExportMapEntry& exp = ExportMap[i];

            // Create
            auto obj = ConstructObject(ResolveObjectIndex(exp.ClassIndex));
            obj->Name = CreateFNameFromMappedName(exp.ObjectName).Text();
            obj->Owner = this;
            obj->Outer = dynamic_cast<ResolvedExportObject*>(ResolveObjectIndex(exp.OuterIndex));
            if (obj->Outer == nullptr) obj->Outer = GetSelfResolved();
            obj->Super = dynamic_cast<ResolvedExportObject*>(ResolveObjectIndex(exp.SuperIndex));
            obj->Template = dynamic_cast<ResolvedExportObject*>(ResolveObjectIndex(exp.TemplateIndex));
            // We give loaded objects the RF_WasLoaded flag in ConstructObject, so don't remove it again in here
            obj->Flags = static_cast<Exports::EObjectFlags>(obj->Flags | exp.ObjectFlags);

            // Serialize (clone the header archive so the seek/read is independent)
            auto arBase = _uassetAr->Clone();
            auto* ar = static_cast<FAssetArchive*>(arBase.get());
            ar->AbsoluteOffset = info.NewPos
                ? _cookedHeaderSize - _allExportDataOffset
                : static_cast<int>(exp.CookedSerialOffset) - info.Pos;
            ar->Position = info.Pos;
            DeserializeObject(*obj, *ar, static_cast<int64_t>(exp.CookedSerialSize));
            obj->Flags = static_cast<Exports::EObjectFlags>(obj->Flags | Exports::RF_LoadCompleted);
            obj->PostLoad();

            ExportsLazy[i] = std::move(obj);
        }
        return ExportsLazy[i].get();
    }

    const std::vector<IoPackage*>& IoPackage::ImportedPackages()
    {
        if (!_importedPackagesResolved)
        {
            _importedPackagesResolved = true;
            _importedPackages.assign(_importedPackageIds.size(), nullptr);
            if (_provider != nullptr)
            {
                for (size_t i = 0; i < _importedPackageIds.size(); i++)
                    _importedPackages[i] = dynamic_cast<IoPackage*>(_provider->TryLoadPackage(_importedPackageIds[i]));
            }
        }
        return _importedPackages;
    }

    ResolvedObject* IoPackage::GetResolvedExport(int exportIndex)
    {
        if (exportIndex < 0 || exportIndex >= static_cast<int>(ExportMap.size())) return nullptr;
        const size_t i = static_cast<size_t>(exportIndex);
        if (!_exportResolved[i]) _exportResolved[i] = std::make_unique<ResolvedExportObject>(this, exportIndex);
        return _exportResolved[i].get();
    }

    ResolvedObject* IoPackage::GetSelfResolved()
    {
        if (!_selfResolved) _selfResolved = std::make_unique<ResolvedPackageObject>(this);
        return _selfResolved.get();
    }

    ResolvedObject* IoPackage::ResolvePackageIndex(const FPackageIndex* index)
    {
        if (index == nullptr || index->IsNull()) return nullptr;
        if (index->IsImport() && (-index->Index - 1) < static_cast<int>(ImportMap.size()))
            return ResolveObjectIndex(ImportMap[static_cast<size_t>(-index->Index - 1)]);
        if (index->IsExport() && (index->Index - 1) < static_cast<int>(ExportMap.size()))
            return GetResolvedExport(index->Index - 1);
        return nullptr;
    }

    ResolvedObject* IoPackage::ResolveObjectIndex(FPackageObjectIndex index)
    {
        if (index.IsNull()) return nullptr;

        if (index.IsExport())
            return GetResolvedExport(static_cast<int>(index.AsExport()));

        if (index.IsScriptImport())
        {
            const auto entry = _globalData->ScriptObjectEntriesMap.find(index);
            if (entry != _globalData->ScriptObjectEntriesMap.end())
            {
                auto cached = _scriptResolved.find(index);
                if (cached == _scriptResolved.end())
                    cached = _scriptResolved.emplace(index, std::make_unique<ResolvedScriptObject>(this, entry->second)).first;
                return cached->second.get();
            }
        }

        if (index.IsPackageImport())
        {
            const auto packageImportRef = index.AsPackageImportRef();
            const std::vector<IoPackage*>& importedPackages = ImportedPackages();

            if (_hasPublicExportHashes)
            {
                if (packageImportRef.ImportedPackageIndex < importedPackages.size() &&
                    packageImportRef.ImportedPublicExportHashIndex < ImportedPublicExportHashes.size())
                {
                    IoPackage* pkg = importedPackages[packageImportRef.ImportedPackageIndex];
                    const uint64_t hash = ImportedPublicExportHashes[packageImportRef.ImportedPublicExportHashIndex];
                    if (pkg != nullptr)
                    {
                        for (int exportIndex = 0; exportIndex < static_cast<int>(pkg->ExportMap.size()); ++exportIndex)
                        {
                            if (pkg->ExportMap[static_cast<size_t>(exportIndex)].PublicExportHash == hash)
                                return pkg->GetResolvedExport(exportIndex);
                        }
                    }
                    // C# then searches all previous versions of the package (ImportedPackagesAllVersions);
                    // that needs IFileProvider.TryLoadPackages, which is not ported. TODO.
                }
            }
            else
            {
                for (IoPackage* pkg : importedPackages)
                {
                    if (pkg == nullptr) continue;
                    for (int exportIndex = 0; exportIndex < static_cast<int>(pkg->ExportMap.size()); ++exportIndex)
                    {
                        if (pkg->ExportMap[static_cast<size_t>(exportIndex)].GlobalImportIndex == index)
                            return pkg->GetResolvedExport(exportIndex);
                    }
                }
            }
        }

        // C# logs the miss under Globals.WarnMissingImportPackage.
        return nullptr;
    }

    int IoPackage::GetExportIndex(const std::string& name) const
    {
        for (int i = 0; i < static_cast<int>(ExportMap.size()); i++)
        {
            if (CreateFNameFromMappedName(ExportMap[static_cast<size_t>(i)].ObjectName).Text() == name)
                return i;
        }
        return -1;
    }

    CUE4Parse::FileProvider::IFileProvider* IoPackage::GetProvider() const { return _provider; }

    std::string IoPackage::GetIoPackageName(FArchive& uasset)
    {
        FAssetArchive uassetAr(uasset, nullptr);
        const FZenPackageSummary summary(uassetAr);
        const auto nameMap = FNameEntrySerialized::LoadNameBatch(uassetAr);
        const std::string text = FName(summary.Name, nameMap).Text();
        return text.empty() ? text : text.substr(1); // C#: Text[1..]
    }
}
