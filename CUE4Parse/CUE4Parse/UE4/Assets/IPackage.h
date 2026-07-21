// Ported from CUE4Parse/UE4/Assets/IPackage.cs (subset)
// The parts of the package interface the import/export-map reader layer actually needs: the package name,
// the name pool it resolves FNames against, the package flags FAssetArchive inspects, and index resolution.
//
// The full C# interface also exposes the file provider, mappings, the export list, lazy object loading
// (ExportsLazy / FindObject / GetExport) — all of which belong to the object/property layer and are
// deferred. TODO: grow this as those layers are ported.
#pragma once

#include <string>
#include <vector>

#include "../Objects/UObject/FNameEntrySerialized.h"
#include "../Objects/UObject/EPackageFlags.h"

namespace CUE4Parse::UE4::Objects::UObject { class FPackageIndex; }
namespace CUE4Parse::UE4::Assets::Exports { class UObject; }
namespace CUE4Parse::FileProvider { class IFileProvider; }

namespace CUE4Parse::UE4::Assets
{
    class ResolvedObject; // ResolvedObject.h

    class IPackage
    {
    public:
        virtual ~IPackage() = default;

        // The package's own name (used as the outermost object in a resolved path name).
        virtual const std::string& GetName() const = 0;

        // The serialized name pool; FAssetArchive::ReadFName indexes into this.
        virtual const std::vector<CUE4Parse::UE4::Objects::UObject::FNameEntrySerialized>& NameMap() const = 0;

        // Mirrors C# Summary.PackageFlags.HasFlag(flags).
        virtual bool HasFlags(CUE4Parse::UE4::Objects::UObject::EPackageFlags flags) const = 0;

        // Resolves an import/export index to a ResolvedObject (its name/outer/class chain). Returns null for
        // a null index or an out-of-range one.
        virtual ResolvedObject* ResolvePackageIndex(const CUE4Parse::UE4::Objects::UObject::FPackageIndex* index) = 0;

        // Lazily constructs + deserializes the export object at `index` (mirrors C#'s ExportsLazy[index].Value),
        // caching it on the package. Not pure: packages that don't support object loading return null (the
        // default), so lighter IPackage implementations need not override it.
        virtual Exports::UObject* GetExportObject(int index) { return nullptr; }

        // Index of the export whose object name equals `name`, or -1 (C#'s GetExportIndex). Non-pure for the
        // same reason as GetExportObject.
        virtual int GetExportIndex(const std::string& name) const { return -1; }

        // The export named `name` (loaded + deserialized), or null (C#'s GetExportOrNull). Used by
        // FSoftObjectPath sub-path resolution.
        Exports::UObject* GetExportOrNull(const std::string& name)
        {
            const int index = GetExportIndex(name);
            return index >= 0 ? GetExportObject(index) : nullptr;
        }

        // The file provider this package was loaded through (C#'s IPackage.Provider), or null. Non-pure for
        // the same reason as GetExportObject.
        virtual CUE4Parse::FileProvider::IFileProvider* GetProvider() const { return nullptr; }
    };
}
