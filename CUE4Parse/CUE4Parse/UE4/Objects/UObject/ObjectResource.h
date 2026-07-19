// Ported from CUE4Parse/UE4/Objects/UObject/ObjectResource.cs
// FPackageIndex (a signed index into a package's import/export maps) plus the two resource records the
// maps are made of: FObjectExport (objects stored in this package) and FObjectImport (objects referenced
// from other packages).
//
// Deliberate differences from C#:
//   * Name/index *resolution* is wired up (FPackageIndex::Name / ToString resolve through the owning
//     package — see Package), so FObjectExport.ClassName reflects the resolved class name. Object
//     *loading* (FPackageIndex::Load<T> / TryLoad and the ResolvedObject.Object lazy) is still deferred
//     with the object/property layer. TODO.
//   * The WeakReference<ResolvedObject> cache on FPackageIndex is not reproduced; Name/ToString re-resolve
//     each call (functionally equivalent, resolution is cheap for name lookups).
//   * FObjectExport.GetPublicExportHash / GetGlobalImportIndex (which need FPackageId hashing and the
//     resolved path name / FPackageObjectIndex) are omitted for now. TODO.
//   * The FKismetArchive FPackageIndex ctor is omitted until that reader is ported.
#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "../../Assets/Readers/FAssetArchive.h"
#include "../Core/Misc/FGuid.h"
#include "FName.h"

namespace CUE4Parse::UE4::Objects::UObject
{
    using Assets::Readers::FAssetArchive;
    using Assets::IPackage;
    using Core::Misc::FGuid;

    // Wrapper for an index into a linker's ImportMap/ExportMap. > 0 => export at (Index - 1); < 0 =>
    // import at (-Index - 1); 0 => null.
    class FPackageIndex
    {
    public:
        int32_t Index = 0;
        IPackage* Owner = nullptr;

        FPackageIndex() = default;
        FPackageIndex(FAssetArchive& Ar, int32_t index) : Index(index), Owner(Ar.Owner) {}
        explicit FPackageIndex(FAssetArchive& Ar) : Index(Ar.Read<int32_t>()), Owner(Ar.Owner) {}
        FPackageIndex(IPackage* owner, int32_t index) : Index(index), Owner(owner) {}

        bool IsNull() const { return Index == 0; }
        bool IsExport() const { return Index > 0; }
        bool IsImport() const { return Index < 0; }

        // Resolves via the owning package's import/export maps; "None" when there is no owner or the index
        // does not resolve. (No name cache here, unlike C#'s WeakReference-backed one — see header note.)
        std::string Name() const;

        std::string ToString() const;

        bool operator==(const FPackageIndex& other) const { return Index == other.Index && Owner == other.Owner; }
        bool operator!=(const FPackageIndex& other) const { return !(*this == other); }
    };

    // Base for the two resource records: a name and the index of the object's outer.
    struct FObjectResource
    {
        FName ObjectName;
        FPackageIndex OuterIndex;

        virtual ~FObjectResource() = default;
        virtual std::string ToString() const { return ObjectName.Text(); }
    };

    // A UObject stored on disk within this package (an entry in the ExportMap).
    struct FObjectExport : public FObjectResource
    {
        FPackageIndex ClassIndex;
        FPackageIndex SuperIndex;
        FPackageIndex TemplateIndex;
        uint32_t ObjectFlags = 0;
        int64_t SerialSize = 0;
        int64_t SerialOffset = 0;
        bool ForcedExport = false;
        bool NotForClient = false;
        bool NotForServer = false;
        FGuid PackageGuid;
        bool IsInheritedInstance = false;
        uint32_t PackageFlags = 0;
        bool NotAlwaysLoadedForEditorGame = false;
        bool IsAsset = false;
        bool GeneratePublicHash = false;
        int32_t FirstExportDependency = 0;
        int32_t SerializationBeforeSerializationDependencies = 0;
        int32_t CreateBeforeSerializationDependencies = 0;
        int32_t SerializationBeforeCreateDependencies = 0;
        int32_t CreateBeforeCreateDependencies = 0;
        int64_t ScriptSerializationStartOffset = 0;
        int64_t ScriptSerializationEndOffset = 0;
        std::optional<uint64_t> PublicExportHash;

        std::string ClassName;

        FObjectExport() = default;
        explicit FObjectExport(FAssetArchive& Ar);

        std::string ToString() const override { return ObjectName.Text() + " (" + ClassIndex.Name() + ")"; }
    };

    // A UObject referenced by this package but contained in another (an entry in the ImportMap).
    struct FObjectImport : public FObjectResource
    {
        FName ClassPackage;
        FName ClassName;
        FName PackageName;
        bool ImportOptional = false;

        FObjectImport() = default;
        explicit FObjectImport(FAssetArchive& Ar);
    };
}
