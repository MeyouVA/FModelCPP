// Ported from CUE4Parse/FileProvider/AbstractFileProvider.cs
// The provider base: path fixing, the Files dictionary, and the SaveAsset/CreateReader/LoadPackage
// families all concrete providers share.
//
// Deliberate differences from C#:
//   * The config-ini surface (CustomConfigIni DefaultGame/DefaultEngine, LoadIniConfigs, GameDisplayName,
//     DefaultLightUnit) needs the unported UE4Config layer. TODO with it — PostMount's key verification
//     depends on it too.
//   * LoadVirtualPaths needs UPluginManifest/UPluginDescriptor JSON parsing; deferred. VirtualPaths itself
//     IS ported because FixPath consults it (it just stays empty until then). TODO.
//   * The localization surface beyond the Internationalization lookup table (ChangeCulture / LoadLocalization
//     / GetLanguageCode) needs .locres reading; deferred with it. TODO.
//   * MappingsContainer IS ported (the usmap mappings provider slots in; MappingsForGame resolves through
//     it). The ReadScriptData / ReadShaderMaps / ReadNaniteData flags belong to unported layers and are
//     omitted until something consumes them.
//   * All async variants are dropped (no threading layer), as are the Serilog logs.
//   * C# indexers become GetGameFile(path) / At(path); Try* methods return the value (null/nullopt on
//     failure) instead of bool + out param.
//   * OWNERSHIP: C#'s LoadPackage returns a fresh GC-managed package each call whose archives it keeps
//     alive; here the provider caches the loaded package (plus the archives it reads from) by game-file
//     path and hands out non-owning pointers/references — the C++ Package requires its archives to outlive
//     it, and the provider is the natural owner. Loaded packages live until the provider dies.
//   * LoadPackage's FIoStoreEntry branch (IoPackage) is deferred with the IO Store asset layer; it throws
//     naming the gap rather than misreading a Zen package as a classic one. TODO.
#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "IFileProvider.h"
#include "InternationalizationDictionary.h"
#include "../MappingsProvider/ITypeMappingsProvider.h"
#include "Objects/GameFile.h"
#include "Vfs/FileProviderDictionary.h"
#include "../UE4/Assets/IPackage.h"
#include "../UE4/Versions/VersionContainer.h"
#include "../Utils/StringComparer.h"

namespace CUE4Parse::UE4::Readers { class FArchive; }

namespace CUE4Parse::FileProvider
{
    class AbstractFileProvider : public virtual IFileProvider
    {
    public:
        UE4::Versions::VersionContainer Versions;
        Utils::StringComparer PathComparer;

        // C#'s MappingsContainer: the mappings provider used for unversioned property serialization.
        std::shared_ptr<MappingsProvider::ITypeMappingsProvider> MappingsContainer;

        Vfs::FileProviderDictionary Files;
        InternationalizationDictionary Internationalization;
        // Virtual root -> plugin directory, filled by LoadVirtualPaths (deferred; stays empty until then).
        std::map<std::string, std::string, Utils::StringComparer> VirtualPaths;
        std::map<std::string, std::string, Utils::StringComparer> TextureCachePaths;

        const UE4::Versions::VersionContainer& GetVersions() const override { return Versions; }
        InternationalizationDictionary& GetInternationalization() override { return Internationalization; }
        const MappingsProvider::TypeMappings* MappingsForGame() const override
        { return MappingsContainer ? MappingsContainer->MappingsForGame() : nullptr; }

        // C#'s lazily-computed ProjectName: the first .uproject's root segment, else the first plausible
        // game root, with the MidnightSuns -> CodaGame rename.
        const std::string& ProjectName() const;

        // Normalises user/soft-object paths onto the mounted layout (Game/Engine roots, virtual paths,
        // the FortniteGame GameFeatures fallback).
        std::string FixPath(const std::string& path) const;

        // C#'s this[path]: FixPath + the umap/raw-path fallbacks; throws std::out_of_range on a miss.
        std::shared_ptr<Objects::GameFile> GetGameFile(const std::string& path) const;
        // C#'s TryGetGameFile: null on a miss instead of throwing.
        std::shared_ptr<Objects::GameFile> TryGetGameFile(const std::string& path) const;

        // C#'s SaveAsset/TrySaveAsset.
        std::vector<uint8_t> SaveAsset(const std::string& path) const { return GetGameFile(path)->Read(); }
        std::optional<std::vector<uint8_t>> TrySaveAsset(const std::string& path) const;

        // C#'s CreateReader/TryCreateReader.
        std::unique_ptr<UE4::Readers::FArchive> CreateReader(const std::string& path) const
        { return GetGameFile(path)->CreateReader(); }
        std::unique_ptr<UE4::Readers::FArchive> TryCreateReader(const std::string& path) const;

        // C#'s LoadPackage(string/GameFile). Throws on failure; the reference stays valid for the
        // provider's lifetime (see OWNERSHIP above).
        UE4::Assets::IPackage& LoadPackage(const std::string& path) { return LoadPackage(*GetGameFile(path)); }
        virtual UE4::Assets::IPackage& LoadPackage(Objects::GameFile& file);

        // IFileProvider's TryLoadPackage: null on failure.
        UE4::Assets::IPackage* TryLoadPackage(const std::string& path) override;
        UE4::Assets::IPackage* TryLoadPackage(Objects::GameFile& file);

        // C#'s SavePackage: the package file plus its payload siblings, path -> bytes.
        std::map<std::string, std::vector<uint8_t>> SavePackage(const std::string& path)
        { return SavePackage(*GetGameFile(path)); }
        std::map<std::string, std::vector<uint8_t>> SavePackage(Objects::GameFile& file);

        virtual void Dispose();
        ~AbstractFileProvider() override = default;

    protected:
        explicit AbstractFileProvider(UE4::Versions::VersionContainer versions = UE4::Versions::VersionContainer(),
                                      Utils::StringComparer pathComparer = Utils::StringComparer::Ordinal());

        // C#'s protected TryGetGameFile(path, collection, out file): FixPath, then the umap extension and
        // raw-path fallbacks, against an arbitrary files map (used by the per-archive lookups too).
        std::shared_ptr<Objects::GameFile> TryGetGameFile(
            const std::string& path, const UE4::VirtualFileSystem::GameFileMap& collection) const;

    private:
        // A loaded package plus the archives it lazily re-reads from (which must outlive it).
        struct LoadedPackage
        {
            std::unique_ptr<UE4::Readers::FArchive> UassetAr;
            std::unique_ptr<UE4::Readers::FArchive> UexpAr;
            std::unique_ptr<UE4::Assets::IPackage> Package;
        };

        mutable std::string _projectName; // lazy, like C#'s field
        std::map<std::string, LoadedPackage> _loadedPackages; // keyed by GameFile path
    };
}
