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
        // a null index or an out-of-range one. Object *loading* is deferred (see ResolvedObject.h).
        virtual ResolvedObject* ResolvePackageIndex(const CUE4Parse::UE4::Objects::UObject::FPackageIndex* index) = 0;
    };
}
