// Ported from CUE4Parse/UE4/Assets/Package.cs (header-reading + index-resolution slice).
// Reads a classic .uasset: the FPackageFileSummary, then the name, import and export maps, and resolves
// import/export indices to ResolvedObjects. This is the concrete IPackage the reader layer resolves against.
//
// Deliberate differences from C#:
//   * C#'s Package derives from UObject (AbstractUePackage : UObject). Until the object/property layer
//     exists this port implements IPackage directly and does not derive from UObject.
//   * Object *loading* is deferred: no ExportsLazy / ConstructObject / DeserializeObject / ExportLoader,
//     and no uexp/ubulk/uptnl payload handling. Only the header (summary + name/import/export maps) is read.
//   * The optional summary tables (thumbnails, DependsMap, PreloadDependencies, SoftObjectPaths,
//     DataResourceMap, Trailer) are not read yet. TODO with their element types.
//   * Cross-package import resolution (Provider.TryLoadPackage) is deferred: ResolveImport returns the
//     in-package ResolvedImportObject fallback (which still yields the import's own name), as C# does when
//     no provider/native package is available. TODO once IFileProvider is ported.
//   * Byte-swapped (big-endian) packages throw for now instead of wrapping FArchiveBigEndian. TODO.
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "IPackage.h"
#include "ResolvedObject.h"
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

    class Package : public IPackage
    {
    public:
        FPackageFileSummary Summary;
        std::vector<FNameEntrySerialized> NameMapEntries;
        std::vector<FObjectImport> ImportMap;
        std::vector<FObjectExport> ExportMap;

        // Reads the whole header (summary + name/import/export maps) from a classic .uasset archive.
        explicit Package(FArchive& uasset);

        const std::string& GetName() const override { return _name; }
        const std::vector<FNameEntrySerialized>& NameMap() const override { return NameMapEntries; }
        bool HasFlags(EPackageFlags flags) const override
        {
            return (static_cast<uint32_t>(Summary.PackageFlags) & static_cast<uint32_t>(flags)) != 0;
        }
        ResolvedObject* ResolvePackageIndex(const FPackageIndex* index) override;

        // Index of the export whose object name equals `name` (ordinal), or -1.
        int GetExportIndex(const std::string& name) const;

    private:
        class ResolvedExportObject;
        class ResolvedImportObject;

        ResolvedObject* ResolveImport(const FPackageIndex* index);

        std::string _name;
        // Lazily-built, package-owned ResolvedObjects, cached by import/export array index.
        std::vector<std::unique_ptr<ResolvedObject>> _exportResolved;
        std::vector<std::unique_ptr<ResolvedObject>> _importResolved;
    };
}
