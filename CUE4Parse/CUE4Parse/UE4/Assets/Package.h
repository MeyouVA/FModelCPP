// Ported from CUE4Parse/UE4/Assets/Package.cs (header-reading + index-resolution slice).
// Reads a classic .uasset: the FPackageFileSummary, then the name, import and export maps, and resolves
// import/export indices to ResolvedObjects. This is the concrete IPackage the reader layer resolves against.
//
// Deliberate differences from C#:
//   * C#'s Package derives from UObject (AbstractUePackage : UObject). Until the object/property layer
//     exists this port implements IPackage directly and does not derive from UObject.
//   * Object loading uses only the lazy (useLazySerialization) path: each export is constructed +
//     deserialized on first GetExportObject(i). The dependency-graph ExportLoader path (useLazySerialization
//     == false, with PreloadDependencies) is not ported. ConstructObject is simplified to always build a base
//     UObject (the UStruct/UClass traversal is deferred with those export types). No ubulk/uptnl payloads.
//   * The optional summary tables (thumbnails, DependsMap, PreloadDependencies, SoftObjectPaths,
//     DataResourceMap, Trailer) are not read yet. TODO with their element types.
//   * Cross-package import resolution is ported for classic packages: with a provider, ResolveImport walks to
//     the outermost import package, TryLoadPackage's it, and matches the import by name + outer path name,
//     resolving to the export in *that* package. The IoPackage branch is deferred with IoPackage; script
//     packages ("/Script/...") and misses fall back to the in-package ResolvedImportObject, as in C#.
//   * Byte-swapped (big-endian) packages throw for now instead of wrapping FArchiveBigEndian. TODO.
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "IPackage.h"
#include "ResolvedObject.h"
#include "Exports/UObject.h"
#include "Readers/FAssetArchive.h"
#include "../../FileProvider/IFileProvider.h"
#include "../Objects/UObject/FPackageFileSummary.h"
#include "../Objects/UObject/FNameEntrySerialized.h"
#include "../Objects/UObject/ObjectResource.h"
#include "../Readers/FArchive.h"

namespace CUE4Parse::UE4::Assets
{
    using CUE4Parse::UE4::Objects::UObject::FPackageFileSummary;
    using CUE4Parse::UE4::Objects::UObject::FNameEntrySerialized;
    using CUE4Parse::UE4::Objects::UObject::FObjectImport;
    using CUE4Parse::UE4::Objects::UObject::FObjectExport;
    using CUE4Parse::UE4::Objects::UObject::FPackageIndex;
    using CUE4Parse::UE4::Objects::UObject::EPackageFlags;
    using Readers::FArchive;
    using Readers::FAssetArchive;

    class Package : public IPackage
    {
    public:
        FPackageFileSummary Summary;
        std::vector<FNameEntrySerialized> NameMapEntries;
        std::vector<FObjectImport> ImportMap;
        std::vector<FObjectExport> ExportMap;
        // Lazily-loaded export objects, parallel to ExportMap; a null slot means "not loaded yet".
        std::vector<std::unique_ptr<Exports::UObject>> ExportsLazy;

        // Reads the whole header (summary + name/import/export maps) from a classic .uasset archive. `uexp`
        // is the optional separate export-data archive (.uexp); when null the export data lives in `uasset`.
        // Both archives (and the provider) must outlive the Package (bytes are re-read lazily on
        // GetExportObject; imports resolve through the provider on demand).
        explicit Package(FArchive& uasset, FArchive* uexp = nullptr,
                         CUE4Parse::FileProvider::IFileProvider* provider = nullptr);

        const std::string& GetName() const override { return _name; }
        const std::vector<FNameEntrySerialized>& NameMap() const override { return NameMapEntries; }
        bool HasFlags(EPackageFlags flags) const override
        {
            return (static_cast<uint32_t>(Summary.PackageFlags) & static_cast<uint32_t>(flags)) != 0;
        }
        ResolvedObject* ResolvePackageIndex(const FPackageIndex* index) override;
        Exports::UObject* GetExportObject(int index) override;
        CUE4Parse::FileProvider::IFileProvider* GetProvider() const override { return _provider; }

        // Index of the export whose object name equals `name` (ordinal), or -1.
        int GetExportIndex(const std::string& name) const override;

    private:
        class ResolvedExportObject;
        class ResolvedImportObject;

        ResolvedObject* ResolveImport(const FPackageIndex* index);

        // Builds a base UObject for an export (the C# default ConstructObject path; the UStruct/UClass
        // traversal is deferred), then seeks + deserializes its serial range.
        std::unique_ptr<Exports::UObject> ConstructObject(ResolvedObject* struc, Exports::EObjectFlags flags);
        void DeserializeObject(Exports::UObject& obj, FAssetArchive& Ar, int64_t serialSize);

        // The package-as-outer fallback (C#'s new ResolvedPackageObject(this)), owned here, built on demand.
        ResolvedObject* GetSelfResolved();

        std::string _name;
        CUE4Parse::FileProvider::IFileProvider* _provider = nullptr;
        // Lazily-built, package-owned ResolvedObjects, cached by import/export array index.
        std::vector<std::unique_ptr<ResolvedObject>> _exportResolved;
        std::vector<std::unique_ptr<ResolvedObject>> _importResolved;
        // Import-resolution result cache (may point into ANOTHER package's resolved exports, so it is a
        // non-owning cache separate from the _importResolved fallback storage above).
        std::vector<ResolvedObject*> _importCache;
        // Clonable archive positioned over the export data (uexp, or uasset when there is no uexp).
        std::unique_ptr<FAssetArchive> _exportAr;
        std::unique_ptr<ResolvedObject> _selfResolved;
    };
}
