// Ported from CUE4Parse/UE4/Assets/Package.cs (header-reading + index-resolution slice).
#include "Package.h"

#include <exception>
#include <optional>

#include "Readers/FAssetArchive.h"
#include "ObjectTypeRegistry.h"
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
        // C#: Package.ResolvePackageIndex(OuterIndex) ?? new ResolvedPackageObject(Package) — a root export's
        // outer is the package itself, so export path names include the package name.
        ResolvedObject* Outer() const override
        {
            auto* pkg = static_cast<CUE4Parse::UE4::Assets::Package*>(this->Package);
            ResolvedObject* outer = pkg->ResolvePackageIndex(&_export->OuterIndex);
            return outer != nullptr ? outer : pkg->GetSelfResolved();
        }
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

    Package::Package(FArchive& uasset, FArchive* uexp,
                     FAssetArchive::RawPayloadProvider ubulk, FAssetArchive::RawPayloadProvider uptnl,
                     CUE4Parse::FileProvider::IFileProvider* provider)
        : _provider(provider)
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
        _importCache.resize(ImportMap.size(), nullptr);
        _exportResolved.resize(static_cast<size_t>(Summary.ExportCount));

        uassetAr.SeekAbsolute(Summary.ExportOffset, ESeekOrigin::Begin);
        ExportMap.resize(static_cast<size_t>(Summary.ExportCount));
        for (int i = 0; i < Summary.ExportCount; i++)
            ExportMap[static_cast<size_t>(i)] = FObjectExport(uassetAr);

        ExportsLazy.resize(static_cast<size_t>(Summary.ExportCount));

        // The UE5 bulk-data table. FByteBulkDataHeader reads an index into this instead of an inline header
        // whenever it is non-empty, so it has to be read before any export is deserialized.
        if (Summary.DataResourceOffset > 0)
        {
            uassetAr.SeekAbsolute(Summary.DataResourceOffset, ESeekOrigin::Begin);
            const auto dataResourceVersion =
                static_cast<CUE4Parse::UE4::Objects::UObject::EObjectDataResourceVersion>(uassetAr.Read<uint32_t>());
            if (dataResourceVersion > CUE4Parse::UE4::Objects::UObject::EObjectDataResourceVersion::Invalid &&
                dataResourceVersion <= CUE4Parse::UE4::Objects::UObject::EObjectDataResourceVersion::Latest)
            {
                const int count = uassetAr.Read<int32_t>();
                DataResourceMap.reserve(static_cast<size_t>(count));
                for (int i = 0; i < count; i++)
                    DataResourceMap.emplace_back(uassetAr, dataResourceVersion);
            }
        }

        // The archive to (re-)read export data from on demand. With a separate .uexp its serial offsets are
        // package-global, so AbsoluteOffset = uasset length translates them; without one the data is in the
        // uasset (AbsoluteOffset 0). We keep a clonable copy — each load clones it so it can seek freely.
        _exportAr = std::make_unique<FAssetArchive>(
            uexp != nullptr ? *uexp : uasset, this,
            uexp != nullptr ? static_cast<int>(uassetAr.Length) : 0);

        // Attach ubulk and uptnl. C# hangs these off the *export* archive, which is the one every
        // FByteBulkDataHeader is read through.
        if (ubulk != nullptr)
            _exportAr->AddPayload(Utils::PayloadType::UBULK, Summary.BulkDataStartOffset, std::move(ubulk));
        if (uptnl != nullptr)
            _exportAr->AddPayload(Utils::PayloadType::UPTNL, Summary.BulkDataStartOffset, std::move(uptnl));
    }

    std::unique_ptr<Exports::UObject> Package::ConstructObject(ResolvedObject* struc, Exports::EObjectFlags /*flags*/)
    {
        // C# (AbstractUePackage.ConstructObject) walks struc's UStruct/UClass chain up to a code-defined class,
        // then UClass.ConstructObject looks that class's name up in ObjectTypeRegistry to build the concrete
        // type. We instead key the registry directly on the resolved class object's name — the same string
        // C#'s walk ultimately resolves to for a native class (whose class import ObjectName is e.g. "Texture2D",
        // "DataTable", "BlueprintGeneratedClass"). That handles every native export, including a cooked
        // UBlueprintGeneratedClass whose class import is "BlueprintGeneratedClass".
        //
        // The full SuperStruct-chain walk stays deferred: it only changes the outcome for an *instance* of a
        // Blueprint (class name "SomeBP_C") whose class is another export/import. Walking it requires loading a
        // class reference as a UStruct — and in this port an import does not load to a UStruct/UScriptClass
        // (ResolvedImportObject::Object() is null; the import-class -> UScriptClass mapping is not ported), so
        // the walk would immediately dead-end at a base UObject anyway. It becomes worth porting once a file
        // provider + mappings can actually load those super classes. TODO.
        //
        // Unregistered names (and a null class) fall through to a base UObject, matching C#'s default path.
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

    void Package::DeserializeObject(Exports::UObject& obj, FAssetArchive& Ar, int64_t serialSize)
    {
        if (serialSize == 0) return; // Empty export.
        const int64_t validPos = Ar.Position + serialSize;
        try
        {
            obj.Deserialize(Ar, validPos);
        }
        catch (const std::exception&)
        {
            // C# logs the error (and rethrows only under Globals.FatalObjectSerializationErrors). There is no
            // logging framework here, so a serialization failure leaves the object partially read and is swallowed.
        }
    }

    Exports::UObject* Package::GetExportObject(int index)
    {
        if (index < 0 || index >= static_cast<int>(ExportsLazy.size())) return nullptr;
        const size_t i = static_cast<size_t>(index);
        if (!ExportsLazy[i])
        {
            FObjectExport& exp = ExportMap[i];

            // Create.
            auto obj = ConstructObject(ResolvePackageIndex(&exp.ClassIndex), static_cast<Exports::EObjectFlags>(exp.ObjectFlags));
            obj->Name = exp.ObjectName.Text();
            obj->Owner = this;
            obj->Outer = ResolvePackageIndex(&exp.OuterIndex);
            if (obj->Outer == nullptr)
                obj->Outer = GetSelfResolved();
            obj->Super = ResolvePackageIndex(&exp.SuperIndex);
            obj->Template = ResolvePackageIndex(&exp.TemplateIndex);
            obj->Flags = static_cast<Exports::EObjectFlags>(obj->Flags | exp.ObjectFlags);

            // Serialize (clone the export archive so the seek/read is independent).
            auto arBase = _exportAr->Clone();
            auto* ar = static_cast<FAssetArchive*>(arBase.get());
            ar->SeekAbsolute(exp.SerialOffset, ESeekOrigin::Begin);
            DeserializeObject(*obj, *ar, exp.SerialSize);
            obj->Flags = static_cast<Exports::EObjectFlags>(obj->Flags | Exports::RF_LoadCompleted);
            obj->PostLoad();

            // Keep the clone alive: anything the export read lazily still points into it.
            _exportArClones.push_back(std::move(arBase));

            ExportsLazy[i] = std::move(obj);
        }
        return ExportsLazy[i].get();
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

    ResolvedObject* Package::ResolveImport(const FPackageIndex* importIndex)
    {
        const size_t cacheIdx = static_cast<size_t>(-importIndex->Index - 1);
        if (_importCache[cacheIdx] != nullptr) return _importCache[cacheIdx];

        FObjectImport* import = &ImportMap[cacheIdx];

        // The in-package fallback (C#'s new ResolvedImportObject(import, this)), owned by this package.
        const auto fallback = [&]() -> ResolvedObject* {
            if (!_importResolved[cacheIdx])
                _importResolved[cacheIdx] = std::make_unique<ResolvedImportObject>(this, import);
            return _importResolved[cacheIdx].get();
        };
        const auto finish = [&](ResolvedObject* r) -> ResolvedObject* {
            _importCache[cacheIdx] = r;
            return r;
        };

        // Walk to the outermost import (the package the object lives in).
        FPackageIndex outerMostIndex = *importIndex;
        const FObjectImport* outerMostImport = nullptr;
        while (true)
        {
            // Special case when the outermost import is an export in this package.
            if (outerMostIndex.IsExport())
                return finish(fallback());

            outerMostImport = &ImportMap[static_cast<size_t>(-outerMostIndex.Index - 1)];
            if (outerMostImport->OuterIndex.IsNull())
                break;
            outerMostIndex = outerMostImport->OuterIndex;
        }

        // We don't support loading script packages, so just return the fallback.
        const std::string outerMostName = outerMostImport->ObjectName.Text();
        if (outerMostName.rfind("/Script/", 0) == 0)
            return finish(fallback());

        if (_provider == nullptr)
            return finish(fallback()); // C#: if (Provider is null) return new ResolvedImportObject(...).

        // The IoPackage branch (matching an FExportMapEntry by mapped name) is deferred with IoPackage.
        auto* importPackage = dynamic_cast<Package*>(_provider->TryLoadPackage(outerMostName));
        if (importPackage == nullptr)
            return finish(fallback()); // Missing native package (C# logs in DEBUG).

        // For an import nested below the package level, the outer's resolved path name must match too.
        std::optional<std::string> outer;
        if (outerMostIndex != import->OuterIndex && import->OuterIndex.IsImport())
        {
            ResolvedObject* outerResolved = ResolveImport(&import->OuterIndex);
            if (outerResolved == nullptr)
                return finish(fallback()); // Missing outer (C# logs in DEBUG).
            outer = outerResolved->GetPathName();
        }

        for (size_t i = 0; i < importPackage->ExportMap.size(); i++)
        {
            const FObjectExport& exp = importPackage->ExportMap[i];
            if (exp.ObjectName.Text() != import->ObjectName.Text())
                continue;
            ResolvedObject* thisOuter = importPackage->ResolvePackageIndex(&exp.OuterIndex);
            // C#: thisOuter?.GetPathName() == outer — both null (a root export matching a package-level
            // import) also matches.
            const bool match = outer.has_value()
                ? (thisOuter != nullptr && thisOuter->GetPathName() == *outer)
                : (thisOuter == nullptr);
            if (match)
            {
                // The resolved export lives in (and is owned by) the import package.
                FPackageIndex exportIndex(importPackage, static_cast<int32_t>(i) + 1);
                return finish(importPackage->ResolvePackageIndex(&exportIndex));
            }
        }

        return finish(fallback()); // Import not found in the package (C# logs in DEBUG).
    }

    ResolvedObject* Package::GetSelfResolved()
    {
        if (!_selfResolved) _selfResolved = std::make_unique<ResolvedPackageObject>(this);
        return _selfResolved.get();
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
