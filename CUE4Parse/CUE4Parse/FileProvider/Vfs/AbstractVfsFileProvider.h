// Ported from CUE4Parse/FileProvider/Vfs/AbstractVfsFileProvider.cs
// The register -> (submit key) -> mount lifecycle over pak/utoc readers, plus the per-archive lookups.
//
// Deliberate differences from C#:
//   * The ~30 per-game CustomEncryption delegates C#'s constructor wires from CUE4Parse.GameTypes are not
//     ported, so CustomEncryption starts null for every game. This is an explicit gap, not silent
//     corruption: an encrypted archive for one of those games fails the mount-point probe
//     (InvalidAesKeyException) and simply never mounts, rather than decrypting to wrong bytes. TODO with
//     the GameTypes layer.
//   * Mount/SubmitKeys run serially — C# fans each reader out on Task.Run; the port has no threading
//     layer. The ConcurrentDictionary sets become plain vectors.
//   * C# catches and LOGS non-InvalidAesKey exceptions per reader ("Uncaught exception while loading
//     file"); with no logging layer the port catches and swallows them the same way, so one corrupt
//     archive still cannot abort the whole mount pass.
//   * The .utoc (IoStoreReader) and .uondemandtoc register branches, VerifyGlobalData/GlobalData/FilesById,
//     LoadPackage(FPackageId), ScanForPackageRefs and TryFindStoreEntry arrive with the IO Store layer;
//     PostMount needs the config-ini layer; UnloadNonStreamedVfs's StreamedGameFile check is reduced to
//     OsGameFile (StreamedGameFile is unported). TODO.
//   * The RegisterVfs stream-array overloads collapse into RegisterVfs(shared_ptr<FArchive>): C#'s five
//     overloads all exist to wrap OS/stream handles into archives, which callers do directly here.
//   * The FragPunk special case in SubmitKeys ("global" containers get the key pre-assigned) is kept.
//   * GetArchive throws std::out_of_range for KeyNotFoundException; TryGetArchive returns null instead of
//     bool + out.
#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "IVfsFileProvider.h"
#include "../AbstractFileProvider.h"
#include "../../UE4/IO/IoStoreReader.h"
#include "../../UE4/VirtualFileSystem/AbstractAesVfsReader.h"

namespace CUE4Parse::FileProvider::Vfs
{
    using UE4::VirtualFileSystem::AbstractAesVfsReader;
    using UE4::VirtualFileSystem::VfsEventHandler;
    using Encryption::Aes::FAesKey;

    class AbstractVfsFileProvider : public AbstractFileProvider, public IVfsFileProvider
    {
    public:
        const std::vector<std::shared_ptr<AbstractAesVfsReader>>& UnloadedVfs() const { return _unloadedVfs; }
        const std::vector<std::shared_ptr<AbstractAesVfsReader>>& MountedVfs() const { return _mountedVfs; }

        const std::map<FGuid, std::shared_ptr<FAesKey>, FGuidLess>& Keys() const { return _keys; }
        const std::set<FGuid, FGuidLess>& RequiredKeys() const { return _requiredKeys; }

        int LooseFileCount = 0;

        // The per-game decryption hook handed to every registered reader (see header comment).
        UE4::VirtualFileSystem::CustomEncryptionDelegate CustomEncryption;

        // C#'s events (single handler each, no multicast).
        VfsEventHandler VfsRegistered;
        VfsEventHandler VfsMounted;
        VfsEventHandler VfsUnmounted;

        // Opens `file` and registers a reader for it by extension (.pak, .utoc; .upak for LordOfMysteries).
        // Failures are swallowed like C#'s catch-and-log.
        void RegisterVfs(const std::string& file) override;
        // Registers a reader over an already-open archive (C#'s stream overloads). For a .utoc,
        // `openContainerStreamFunc` supplies the .ucas partition archives; when null the partitions are
        // opened from disk next to the toc's name.
        void RegisterVfs(std::shared_ptr<UE4::Readers::FArchive> archive,
                         UE4::IO::OpenContainerStreamFunc openContainerStreamFunc = nullptr);

        int Mount() override;
        int SubmitKey(const FGuid& guid, const std::shared_ptr<FAesKey>& key) override
        {
            return SubmitKeys({{guid, key}});
        }
        int SubmitKeys(const std::vector<std::pair<FGuid, std::shared_ptr<FAesKey>>>& keys) override;

        // C#'s GetArchive/TryGetArchive: mounted first, then unloaded.
        AbstractAesVfsReader& GetArchive(const std::string& archiveName,
                                         const Utils::StringComparer& comparison = Utils::StringComparer::Ordinal());
        IAesVfsReader* TryGetArchive(const std::string& archiveName,
                                     const Utils::StringComparer& comparison = Utils::StringComparer::Ordinal()) override;

        // C#'s this[path, archive] / TryGetGameFile(path, archiveName): a file looked up inside one
        // specific archive. The usings keep the whole-provider base overloads visible.
        using AbstractFileProvider::GetGameFile;
        using AbstractFileProvider::TryGetGameFile;
        std::shared_ptr<Objects::GameFile> GetGameFile(const std::string& path, const IAesVfsReader& archive) const;
        std::shared_ptr<Objects::GameFile> TryGetGameFile(const std::string& path, const std::string& archiveName,
                                                          const Utils::StringComparer& comparison = Utils::StringComparer::Ordinal());

        void UnloadAllVfs();
        void UnloadNonStreamedVfs();

        void Dispose() override;
        ~AbstractVfsFileProvider() override = default;

    protected:
        explicit AbstractVfsFileProvider(UE4::Versions::VersionContainer versions = UE4::Versions::VersionContainer(),
                                         Utils::StringComparer pathComparer = Utils::StringComparer::Ordinal());

        // Registers a constructed reader: tracks its key requirement, hands it the custom-encryption hook,
        // and fires VfsRegistered.
        void PostLoadReader(std::shared_ptr<AbstractAesVfsReader> reader, bool isConcurrent = true);

        std::vector<std::shared_ptr<AbstractAesVfsReader>> _unloadedVfs;
        std::vector<std::shared_ptr<AbstractAesVfsReader>> _mountedVfs;
        std::map<FGuid, std::shared_ptr<FAesKey>, FGuidLess> _keys;
        std::set<FGuid, FGuidLess> _requiredKeys;
    };
}
