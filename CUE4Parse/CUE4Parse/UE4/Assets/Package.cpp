// Ported from CUE4Parse/UE4/Assets/Package.cs (header-reading + index-resolution slice).
#include "Package.h"

#include "Readers/FAssetArchive.h"
#include "../Exceptions/ParserException.h"

namespace CUE4Parse::UE4::Assets
{
    using Readers::FAssetArchive;
    using Readers::ESeekOrigin;

    // A resolved entry from this package's ExportMap.
    class Package::ResolvedExportObject : public ResolvedObject
    {
    public:
        // Enclosing type is qualified: inside this derived class `Package` alone names the inherited member.
        ResolvedExportObject(CUE4Parse::UE4::Assets::Package* package, int exportIndex)
            : ResolvedObject(package, exportIndex), _export(&package->ExportMap[exportIndex]) {}

        FName Name() const override { return _export->ObjectName; }
        // `this->Package` disambiguates the inherited IPackage* member from the enclosing Package type name.
        ResolvedObject* Outer() const override { return this->Package->ResolvePackageIndex(&_export->OuterIndex); }
        ResolvedObject* Class() const override { return this->Package->ResolvePackageIndex(&_export->ClassIndex); }
        ResolvedObject* Super() const override { return this->Package->ResolvePackageIndex(&_export->SuperIndex); }

    private:
        FObjectExport* _export;
    };

    // A resolved entry from this package's ImportMap (in-package fallback; cross-package resolution deferred).
    class Package::ResolvedImportObject : public ResolvedObject
    {
    public:
        ResolvedImportObject(CUE4Parse::UE4::Assets::Package* package, FObjectImport* import)
            : ResolvedObject(package), _import(import) {}

        FName Name() const override { return _import->ObjectName; }
        ResolvedObject* Outer() const override { return this->Package->ResolvePackageIndex(&_import->OuterIndex); }
        // Class() would be a ResolvedLoadedObject(UScriptClass) in C#; that needs UObject, so deferred.

    private:
        FObjectImport* _import;
    };

    Package::Package(FArchive& uasset)
    {
        _name = uasset.Name();
        const auto dot = _name.find_last_of('.');
        if (dot != std::string::npos) _name = _name.substr(0, dot);

        FAssetArchive uassetAr(uasset, this);

        // The package may be stored byte-swapped relative to this (little-endian) linker.
        const uint32_t tag = uassetAr.Read<uint32_t>();
        if (tag == FPackageFileSummary::PACKAGE_FILE_TAG_SWAPPED)
            throw Exceptions::ParserException(uassetAr, "Byte-swapped (big-endian) packages are not yet supported");
        uassetAr.Position -= 4;

        Summary = FPackageFileSummary(uassetAr);

        uassetAr.SeekAbsolute(Summary.NameOffset, ESeekOrigin::Begin);
        NameMapEntries.reserve(static_cast<size_t>(Summary.NameCount));
        for (int i = 0; i < Summary.NameCount; i++)
            NameMapEntries.emplace_back(uassetAr);

        uassetAr.SeekAbsolute(Summary.ImportOffset, ESeekOrigin::Begin);
        ImportMap.reserve(static_cast<size_t>(Summary.ImportCount));
        for (int i = 0; i < Summary.ImportCount; i++)
            ImportMap.emplace_back(uassetAr);

        // Size the resolution caches before reading exports: FObjectExport's ctor resolves ClassIndex.Name,
        // which indexes into these. (Imports are fully read by now; a forward export ref reads a default,
        // not-yet-filled ExportMap entry, matching C#'s pre-sized ExportMap.)
        _importResolved.resize(ImportMap.size());
        _exportResolved.resize(static_cast<size_t>(Summary.ExportCount));

        uassetAr.SeekAbsolute(Summary.ExportOffset, ESeekOrigin::Begin);
        ExportMap.resize(static_cast<size_t>(Summary.ExportCount));
        for (int i = 0; i < Summary.ExportCount; i++)
            ExportMap[static_cast<size_t>(i)] = FObjectExport(uassetAr);
    }

    ResolvedObject* Package::ResolvePackageIndex(const FPackageIndex* index)
    {
        if (index == nullptr || index->IsNull()) return nullptr;
        if (index->IsImport() && (-index->Index - 1) < static_cast<int>(ImportMap.size()))
            return ResolveImport(index);
        if (index->IsExport() && (index->Index - 1) < static_cast<int>(ExportMap.size()))
        {
            const size_t i = static_cast<size_t>(index->Index - 1);
            if (!_exportResolved[i]) _exportResolved[i] = std::make_unique<ResolvedExportObject>(this, static_cast<int>(i));
            return _exportResolved[i].get();
        }
        return nullptr;
    }

    ResolvedObject* Package::ResolveImport(const FPackageIndex* index)
    {
        const size_t i = static_cast<size_t>(-index->Index - 1);
        if (!_importResolved[i]) _importResolved[i] = std::make_unique<ResolvedImportObject>(this, &ImportMap[i]);
        return _importResolved[i].get();
    }

    int Package::GetExportIndex(const std::string& name) const
    {
        for (int i = 0; i < static_cast<int>(ExportMap.size()); i++)
        {
            if (ExportMap[static_cast<size_t>(i)].ObjectName.Text() == name)
                return i;
        }
        return -1;
    }
}
