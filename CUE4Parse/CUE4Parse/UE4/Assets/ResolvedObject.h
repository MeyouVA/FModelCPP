// Ported from the ResolvedObject hierarchy in CUE4Parse/UE4/Assets/AbstractUePackage.cs.
// A ResolvedObject is the resolution of an FPackageIndex against a package's import/export maps: it knows
// the object's Name and its Outer/Class/Super chain, which is enough to print full path names.
//
// Deliberate differences from C#:
//   * Object *loading* is deferred with the object/property layer. The C# `Object` (Lazy<UObject>) and the
//     Load/TryLoad helpers are omitted; a ResolvedObject here answers names and the resolution chain only.
//   * ResolvedLoadedObject (which wraps an already-loaded UObject) is omitted until UObject exists, so
//     ResolvedImportObject::Class returns null rather than a UScriptClass wrapper. TODO.
#pragma once

#include <string>

#include "../Objects/UObject/FName.h"

namespace CUE4Parse::UE4::Assets
{
    class IPackage;
    using CUE4Parse::UE4::Objects::UObject::FName;

    class ResolvedObject
    {
    public:
        IPackage* Package;
        int ExportIndex;

        explicit ResolvedObject(IPackage* package, int exportIndex = -1)
            : Package(package), ExportIndex(exportIndex) {}
        virtual ~ResolvedObject() = default;

        virtual FName Name() const = 0;
        virtual ResolvedObject* Outer() const { return nullptr; }
        virtual ResolvedObject* Class() const { return nullptr; }
        virtual ResolvedObject* Super() const { return nullptr; }

        std::string GetFullName(bool includeOuterMostName = true, bool includeClassPackage = false) const;
        void GetFullName(bool includeOuterMostName, std::string& result, bool includeClassPackage = false) const;
        std::string GetPathName(bool includeOuterMostName = true) const;
        void GetPathName(bool includeOuterMostName, std::string& result) const;

        std::string ToString() const { return GetFullName(); }
    };

    // A ResolvedObject standing in for the package itself (the outermost outer of every export).
    class ResolvedPackageObject : public ResolvedObject
    {
    public:
        explicit ResolvedPackageObject(IPackage* package) : ResolvedObject(package) {}
        FName Name() const override;
    };
}
