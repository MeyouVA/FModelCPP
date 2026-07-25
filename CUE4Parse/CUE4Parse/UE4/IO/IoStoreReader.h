// Ported from CUE4Parse/UE4/IO/IoStoreReader.cs
// Mounts a .utoc/.ucas pair (or partitioned _sN.ucas set): parses the toc, walks the directory index into
// Files, and extracts chunks by rebuilding them from their compression blocks.
//
// Deliberate differences from C#:
//   * ContainerHeader/IoGlobalData (the FIoContainerHeader chunk, script objects, global names) belong to
//     the unported Zen/IoPackage asset layer; ContainerHeader() throws naming that gap instead of quietly
//     returning null. InitializeContainerHeader is a no-op until then. TODO.
//   * The GAME_eBaseballProSpirit block-trailer arithmetic needs GameTypes' ProSpiEncryption; extraction
//     for that game throws naming it. Every other per-game branch here (TheFinals/ArcRaiders, NFS Mobile,
//     FragPunk, LordOfMysteries, NeedForSpeedMobile paths, MindsEye partial encryption) is self-contained
//     and IS ported.
//   * TocImperfectHashMapFallback keys on (ChunkId, ChunkType) — exactly the fields C#'s
//     FIoChunkId.Equals compares (it ignores the chunk index).
//   * C#'s char[]-pool path building in ProcessIndex becomes plain std::string concatenation.
#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Objects/FIoChunkId.h"
#include "Objects/FIoOffsetAndLength.h"
#include "Objects/FIoContainerHeader.h"
#include "Objects/FIoStoreTocResource.h"
#include "Objects/FPackageId.h"
#include "../Readers/FArchive.h"
#include "../VirtualFileSystem/AbstractAesVfsReader.h"

namespace CUE4Parse::UE4::IO
{
    using OpenContainerStreamFunc = std::function<std::shared_ptr<Readers::FArchive>(const std::string&)>;

    class IoStoreReader : public VirtualFileSystem::AbstractAesVfsReader
    {
    public:
        std::vector<std::shared_ptr<Readers::FArchive>> ContainerStreams;
        Objects::FIoStoreTocResource TocResource;

        // C#'s Dictionary<FIoChunkId, FIoOffsetAndLength> keyed the way FIoChunkId.Equals compares.
        struct ChunkIdKeyLess
        {
            bool operator()(const Objects::FIoChunkId& a, const Objects::FIoChunkId& b) const
            {
                if (a.ChunkId != b.ChunkId) return a.ChunkId < b.ChunkId;
                return a.ChunkType < b.ChunkType;
            }
        };
        std::optional<std::map<Objects::FIoChunkId, Objects::FIoOffsetAndLength, ChunkIdKeyLess>> TocImperfectHashMapFallback;

        // Files that are UE packages, by their package id (fed while walking the directory index).
        std::map<Objects::FPackageId, std::shared_ptr<FileProvider::Objects::GameFile>> PackageIdIndex;

        IoStoreReader(std::shared_ptr<Readers::FArchive> tocStream, OpenContainerStreamFunc openContainerStreamFunc,
                      Objects::EIoStoreTocReadOptions readOptions = Objects::EIoStoreTocReadOptions::ReadDirectoryIndex);
        // Opens `tocPath` (and its .ucas partitions) from disk, like C#'s string/FileInfo constructors.
        explicit IoStoreReader(const std::string& tocPath,
                               Objects::EIoStoreTocReadOptions readOptions = Objects::EIoStoreTocReadOptions::ReadDirectoryIndex,
                               Versions::VersionContainer versions = Versions::VersionContainer());

        int64_t Length() const override { return _length; }
        bool HasDirectoryIndex() const override { return TocResource.DirectoryIndexBufferOffset != -1; }
        UE4::Objects::Core::Misc::FGuid EncryptionKeyGuid() const override { return TocResource.Header->EncryptionKeyGuid; }
        bool IsEncrypted() const override
        {
            return Objects::HasFlag(TocResource.Header->ContainerFlags, Objects::EIoContainerFlags::Encrypted);
        }

        // C#'s lazy FIoContainerHeader accessor: reads the ContainerHeader chunk on first call. On a pre-UE5
        // game a read failure returns null (C# swallows there); on UE5+ it rethrows. The result is cached
        // (including a cached null).
        Objects::FIoContainerHeader* ContainerHeader();

        bool DoesChunkExist(const Objects::FIoChunkId& chunkId) { Objects::FIoOffsetAndLength ol; return TryResolve(chunkId, ol); }
        bool TryResolve(const Objects::FIoChunkId& chunkId, Objects::FIoOffsetAndLength& outOffsetLength);

        // The whole chunk with the given id; throws when it is not in this container.
        std::vector<uint8_t> Read(const Objects::FIoChunkId& chunkId);

        std::vector<uint8_t> Extract(VirtualFileSystem::VfsEntry& entry, const Assets::Objects::FByteBulkDataHeader* header = nullptr) override;
        void Mount(const Utils::StringComparer& pathComparer) override;
        std::vector<uint8_t> MountPointCheckBytes() override;

    protected:
        std::vector<uint8_t> ReadAndDecrypt(int length) override;

    private:
        bool TryResolveImperfect(const Objects::FIoChunkId& chunkId, Objects::FIoOffsetAndLength& outOffsetLength);
        std::vector<uint8_t> Read(int64_t offset, int64_t length, int64_t offsetInFile = 0);
        std::vector<uint8_t> ReadPartiallyEncrypted(int64_t offset, int64_t length, int64_t offsetInFile);
        void ProcessIndex(const Utils::StringComparer& pathComparer);

        int64_t _length = 0;
        // C#'s Lazy<FIoContainerHeader?>: read once on first ContainerHeader() call; a cached null means
        // "tried and unavailable" (pre-UE5 read failure).
        std::unique_ptr<Objects::FIoContainerHeader> _containerHeader;
        bool _containerHeaderRead = false;
    };
}
