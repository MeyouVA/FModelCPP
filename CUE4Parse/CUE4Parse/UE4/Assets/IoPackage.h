// Ported from CUE4Parse/UE4/Assets/IoPackage.cs
// A Zen (IO Store) package: the UE5 FZenPackageSummary path and the UE4.26-4.27 FPackageSummary path, the
// name batch, imported public export hashes, the FPackageObjectIndex import map and FExportMapEntry export
// map, then the export-bundle walk that gives every export its serial position.
//
// Deliberate differences from C#:
//   * C#'s IoPackage derives from AbstractUePackage : UObject. This port has no AbstractUePackage (see
//     Package.h for the same note), so IoPackage implements IPackage directly and duplicates the small
//     ConstructObject/DeserializeObject helpers Package also has.
//   * Export loading uses the lazy path only: the bundle walk records each export's (position, newPos) and
//     GetExportObject(i) constructs + deserializes on first use, exactly like Package.
//   * ImportedPackagesAllVersions (C#'s "search all previous versions" fallback in ResolveObjectIndex) needs
//     IFileProvider.TryLoadPackages, which is not ported; package-import resolution therefore searches only
//     the directly imported package. The classic-Package (ResolvedPakExportObject) arm of that fallback is
//     dropped with it. TODO with TryLoadPackages.
//   * ubulk/uptnl payloads are not attached: FAssetArchive's payload subsystem is still deferred (see
//     FAssetArchive.h). The parameters exist so callers match C#, but the archives are unused. TODO.
//   * ResolvedScriptObject::Object IS ported (a cached UScriptClass named after the script import) because
//     unversioned deserialization resolves an export's class through it. Its Class would be a
//     ResolvedLoadedObject(UScriptClass("Class")) in C#; ResolvedLoadedObject is not ported (see
//     ResolvedObject.h), so it stays null — nothing here reads a script object's own class.
//   * C# reads the import/export maps off the *base* archive (`uasset.ReadArray`) after seeking `uassetAr`;
//     that works only because C#'s FAssetArchive delegates Position to the base. This port's FAssetArchive
//     owns its own Position, so both reads go through `uassetAr`.
//   * Log.Warning for a missing store entry / missing import is dropped (no logging layer).
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "IPackage.h"
#include "ResolvedObject.h"
#include "Exports/UObject.h"
#include "Readers/FAssetArchive.h"
#include "../IO/IoGlobalData.h"
#include "../IO/Objects/FBulkDataMapEntry.h"
#include "../IO/Objects/FExportBundleEntry.h"
#include "../IO/Objects/FExportBundleHeader.h"
#include "../IO/Objects/FExportMapEntry.h"
#include "../IO/Objects/FIoContainerHeader.h"
#include "../IO/Objects/FMappedName.h"
#include "../IO/Objects/FPackageObjectIndex.h"
#include "../Objects/UObject/FPackageFileSummary.h"
#include "../Objects/UObject/FNameEntrySerialized.h"
#include "../Objects/UObject/ObjectResource.h"
#include "../Readers/FArchive.h"

namespace CUE4Parse::FileProvider::Vfs { class IVfsFileProvider; }

namespace CUE4Parse::UE4::Assets
{
    using CUE4Parse::UE4::Objects::UObject::FPackageFileSummary;
    using CUE4Parse::UE4::Objects::UObject::FNameEntrySerialized;
    using CUE4Parse::UE4::Objects::UObject::FPackageIndex;
    using CUE4Parse::UE4::Objects::UObject::EPackageFlags;
    using CUE4Parse::UE4::Objects::UObject::FName;
    using Readers::FArchive;
    using Readers::FAssetArchive;

    class IoPackage : public IPackage
    {
    public:
        FPackageFileSummary Summary;
        std::vector<FNameEntrySerialized> NameMapEntries;
        std::vector<uint64_t> ImportedPublicExportHashes;
        std::vector<IO::Objects::FPackageObjectIndex> ImportMap;
        std::vector<IO::Objects::FExportMapEntry> ExportMap;
        std::vector<IO::Objects::FBulkDataMapEntry> BulkDataMap;
        // Lazily-loaded export objects, parallel to ExportMap; a null slot means "not loaded yet".
        std::vector<std::unique_ptr<Exports::UObject>> ExportsLazy;
        bool IsFullyLoaded = false;

        // `uasset` (and the provider) must outlive the package: export data is re-read lazily, and imports
        // resolve through the provider on demand. `containerHeader` may be null (the store entry is then
        // looked up through the provider, as in C#).
        explicit IoPackage(FArchive& uasset,
                           IO::Objects::FIoContainerHeader* containerHeader = nullptr,
                           FArchive* ubulk = nullptr,
                           FArchive* uptnl = nullptr,
                           CUE4Parse::FileProvider::Vfs::IVfsFileProvider* provider = nullptr);

        const std::string& GetName() const override { return _name; }
        const std::vector<FNameEntrySerialized>& NameMap() const override { return NameMapEntries; }
        bool HasFlags(EPackageFlags flags) const override
        {
            return (static_cast<uint32_t>(Summary.PackageFlags) & static_cast<uint32_t>(flags)) != 0;
        }
        ResolvedObject* ResolvePackageIndex(const FPackageIndex* index) override;
        Exports::UObject* GetExportObject(int index) override;
        CUE4Parse::FileProvider::IFileProvider* GetProvider() const override;
        int GetExportIndex(const std::string& name) const override;

        // C#'s ResolveObjectIndex: export / script import / package import.
        ResolvedObject* ResolveObjectIndex(IO::Objects::FPackageObjectIndex index);

        FName CreateFNameFromMappedName(IO::Objects::FMappedName mappedName) const
        {
            return FName(mappedName, mappedName.IsGlobal() ? _globalData->GlobalNameMap : NameMapEntries);
        }

        // C#'s static GetIoPackageName: the package's own name from a Zen summary + name batch, without
        // reading the rest of the header.
        static std::string GetIoPackageName(FArchive& uasset);

    private:
        class ResolvedExportObject;
        class ResolvedScriptObject;

        // Where an export's serial data starts, filled by the export-bundle walk. `Valid` is false for an
        // export the bundles never issue a Serialize command for (C# leaves its Lazy null).
        struct ExportLoadInfo
        {
            bool Valid = false;
            int Pos = 0;
            bool NewPos = false;
        };

        // C#'s (storeEntry, importedPackageIds) tuple.
        void GetStoreEntryAndImportedPackageIds(IO::Objects::FIoContainerHeader* containerHeader,
                                                const IO::Objects::FFilePackageStoreEntry*& outStoreEntry,
                                                std::vector<IO::Objects::FPackageId>& outImportedPackageIds);
        // C#'s LoadExportBundles / LoadGraphData (the pre-UE5 path).
        void LoadExportBundles(FArchive& Ar, int graphDataSize,
                               std::vector<IO::Objects::FExportBundleHeader>& bundleHeaders,
                               std::vector<IO::Objects::FExportBundleEntry>& bundleEntries);
        static std::vector<IO::Objects::FPackageId> LoadGraphData(FArchive& Ar);

        // C#'s AbstractUePackage.CanDeserialize: throws when the package is unversioned and no mappings are
        // available, otherwise true.
        bool CanDeserialize() const;

        std::unique_ptr<Exports::UObject> ConstructObject(ResolvedObject* struc);
        void DeserializeObject(Exports::UObject& obj, FAssetArchive& Ar, int64_t serialSize);

        const std::vector<IoPackage*>& ImportedPackages();
        ResolvedObject* GetResolvedExport(int exportIndex);
        ResolvedObject* GetSelfResolved();

        std::string _name;
        CUE4Parse::FileProvider::Vfs::IVfsFileProvider* _provider = nullptr;
        IO::IoGlobalData* _globalData = nullptr;
        std::unique_ptr<FAssetArchive> _uassetAr;

        // C#'s `ImportedPublicExportHashes != null`: only the UE5 path fills the array, and the null/non-null
        // distinction (not emptiness) picks the package-import resolution strategy.
        bool _hasPublicExportHashes = false;

        std::vector<IO::Objects::FPackageId> _importedPackageIds;
        std::vector<IoPackage*> _importedPackages;
        bool _importedPackagesResolved = false;

        std::vector<ExportLoadInfo> _exportLoadInfo;
        int _cookedHeaderSize = 0;
        int _allExportDataOffset = 0;

        // Package-owned ResolvedObjects (C# news one up per call and lets the GC own it).
        std::vector<std::unique_ptr<ResolvedObject>> _exportResolved;
        std::map<IO::Objects::FPackageObjectIndex, std::unique_ptr<ResolvedObject>> _scriptResolved;
        std::unique_ptr<ResolvedObject> _selfResolved;
    };
}
