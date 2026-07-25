// Ported from CUE4Parse/FileProvider/IFileProvider.cs (minimal subset).
// The provider is what a package resolves *outward* through: other packages (cross-package imports) and
// the localization table (FText Base histories).
//
// Deliberate differences from C#:
//   * Only the surface the reader layer consumes is ported: Versions, Internationalization (+ the
//     GetLocalizedString convenience), and TryLoadPackage. The rest of the huge C# interface — the Files
//     dictionary/GameFile, virtual & texture-cache paths, config inis, mappings container, FixPath, the
//     SaveAsset/CreateReader/SavePackage families, LoadPackageObject and the async variants — arrives with
//     its layers (VFS/archives, mappings, exports). TODO.
//   * C#'s `bool TryLoadPackage(string, out IPackage)` becomes `IPackage* TryLoadPackage(const string&)`
//     returning null on failure (no out-params). The provider OWNS the packages it loads (C# leans on GC);
//     callers keep non-owning pointers, so loaded packages must live as long as the provider.
//   * Concrete providers: AbstractFileProvider (path fixing + load helpers), AbstractVfsFileProvider
//     (register/mount/key lifecycle) and DefaultFileProvider (directory scan) are ported; tests may still
//     implement lightweight in-memory ones directly against this interface.
#pragma once

#include <memory>
#include <string>

#include "InternationalizationDictionary.h"
#include "../UE4/Versions/VersionContainer.h"

namespace CUE4Parse::MappingsProvider { class TypeMappings; }
namespace CUE4Parse::FileProvider::Objects { class GameFile; }
namespace CUE4Parse::UE4::Assets { class IPackage; }
namespace CUE4Parse::UE4::Assets::Exports { class UObject; }

namespace CUE4Parse::FileProvider
{
    using CUE4Parse::UE4::Versions::VersionContainer;

    class IFileProvider
    {
    public:
        virtual ~IFileProvider() = default;

        // The version container used during parsing operations.
        virtual const VersionContainer& GetVersions() const = 0;

        // The localized-resources table (namespace -> key -> string).
        virtual InternationalizationDictionary& GetInternationalization() = 0;

        // C#'s MappingsForGame (=> MappingsContainer?.MappingsForGame): the type mappings for unversioned
        // property serialization, or null when no mappings provider is set.
        virtual const CUE4Parse::MappingsProvider::TypeMappings* MappingsForGame() const { return nullptr; }

        // Loads and parses (or returns the cached) package at `path`; null if it cannot be found/parsed.
        virtual UE4::Assets::IPackage* TryLoadPackage(const std::string& path) = 0;

        // C#'s TryGetGameFile: the mounted file at `path`, or null. Non-pure so the lightweight in-memory
        // providers the tests define need not implement a file table; the bulk-data layer uses it to reach
        // the .ubulk / .uptnl / .m.ubulk sidecars of a cooked-index payload.
        virtual std::shared_ptr<Objects::GameFile> TryGetGameFile(const std::string& path) const { return nullptr; }

        // C# convenience: the localized string for namespace/key, or defaultValue.
        std::string GetLocalizedString(const std::string& ns, const std::string& key, const std::string& defaultValue)
        {
            return GetInternationalization().SafeGet(ns, key, defaultValue);
        }

        // Splits `path` into (package path, object name) like C#'s GetPathName (on the last '.', else the
        // segment after the last '/'), loads that package, and returns its named export (loaded +
        // deserialized), or null. Defined in IFileProvider.cpp. Mirrors C#'s LoadPackageObject path used by
        // TryLoadPackageObject.
        UE4::Assets::Exports::UObject* LoadPackageObject(const std::string& path);

        // C#'s TryLoadPackageObject<T>: the named export cast to T*, or null if missing / not a T. T must be a
        // complete type deriving UObject at the call site (dynamic_cast).
        template <typename T>
        T* TryLoadPackageObject(const std::string& path)
        {
            return dynamic_cast<T*>(LoadPackageObject(path));
        }
    };
}
