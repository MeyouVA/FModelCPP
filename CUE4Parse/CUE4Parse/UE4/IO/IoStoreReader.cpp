#include "IoStoreReader.h"

#include "../Assets/Objects/FByteBulkDataHeader.h"

#include <algorithm>
#include <cstring>
#include <utility>

#include "Objects/FIoDirectoryIndexEntry.h"
#include "Objects/FIoFileIndexEntry.h"
#include "Objects/FIoStatus.h"
#include "Objects/FIoStoreEntry.h"
#include "../Exceptions/InvalidAesKeyException.h"
#include "../Exceptions/ParserException.h"
#include "../Readers/FByteArchive.h"
#include "../Readers/FStreamArchive.h"
#include "../Versions/EGame.h"
#include "../../Compression/Compression.h"
#include "../../Encryption/Aes/Aes.h"
#include "../../Utils/AlignUtils.h"
#include "../../Utils/StringUtils.h"

namespace CUE4Parse::UE4::IO
{
    using namespace CUE4Parse::UE4::Versions;
    using VirtualFileSystem::GameFileMap;
    using CUE4Parse::Encryption::Aes::Aes;
    using Objects::EIoStoreTocVersion;
    using Objects::FIoChunkId;
    using Objects::FIoOffsetAndLength;
    using Objects::FIoStoreEntry;

    IoStoreReader::IoStoreReader(std::shared_ptr<Readers::FArchive> tocStream,
                                 OpenContainerStreamFunc openContainerStreamFunc,
                                 Objects::EIoStoreTocReadOptions readOptions)
        : AbstractAesVfsReader(tocStream->Name(), tocStream->Versions),
          TocResource(tocStream, readOptions)
    {
        _length = tocStream->Length;
        CompressionMethods() = TocResource.CompressionMethods;

        if (TocResource.Header->PartitionCount <= 1)
        {
            try
            {
                ContainerStreams.push_back(openContainerStreamFunc(
                    Utils::SubstringBeforeLast(tocStream->Name(), '.') + ".ucas"));
            }
            catch (const std::exception&)
            {
                throw Objects::FIoStatusException(Objects::EIoErrorCode::FileOpenFailed,
                    "Failed to open container partition 0 for " + tocStream->Name());
            }
        }
        else
        {
            const std::string environmentPath = Utils::SubstringBeforeLast(tocStream->Name(), '.');
            for (uint32_t i = 0; i < TocResource.Header->PartitionCount; i++)
            {
                try
                {
                    const std::string path = i > 0
                        ? environmentPath + "_s" + std::to_string(i) + ".ucas"
                        : environmentPath + ".ucas";
                    ContainerStreams.push_back(openContainerStreamFunc(path));
                }
                catch (const std::exception&)
                {
                    throw Objects::FIoStatusException(Objects::EIoErrorCode::FileOpenFailed,
                        "Failed to open container partition " + std::to_string(i) + " for " + tocStream->Name());
                }
            }
        }

        for (const auto& stream : ContainerStreams) _length += stream->Length;

        if (TocResource.ChunkPerfectHashSeeds.has_value())
        {
            TocImperfectHashMapFallback.emplace();
            if (TocResource.ChunkIndicesWithoutPerfectHash.has_value())
            {
                for (const int32_t chunkIndexWithoutPerfectHash : *TocResource.ChunkIndicesWithoutPerfectHash)
                {
                    (*TocImperfectHashMapFallback)[TocResource.ChunkIds[chunkIndexWithoutPerfectHash]] =
                        TocResource.ChunkOffsetLengths[chunkIndexWithoutPerfectHash];
                }
            }
        }
        // C# logs a warning for an unsupported toc version here; the port has no logging layer.
    }

    IoStoreReader::IoStoreReader(const std::string& tocPath, Objects::EIoStoreTocReadOptions readOptions,
                                 Versions::VersionContainer versions)
        : IoStoreReader(std::make_shared<Readers::FRandomAccessFileStreamArchive>(tocPath, versions),
                        [versions](const std::string& path)
                        {
                            return std::make_shared<Readers::FRandomAccessFileStreamArchive>(path, versions);
                        },
                        readOptions) {}

    Objects::FIoContainerHeader* IoStoreReader::ContainerHeader()
    {
        if (!_containerHeaderRead)
        {
            _containerHeaderRead = true;
            try
            {
                const FIoChunkId headerChunkId(
                    TocResource.Header->ContainerId.Id, 0,
                    Game() >= Versions::GAME_UE5_0 ? static_cast<uint8_t>(Objects::EIoChunkType5::ContainerHeader)
                                                   : static_cast<uint8_t>(Objects::EIoChunkType::ContainerHeader));
                Readers::FByteArchive Ar("ContainerHeader", Read(headerChunkId), GetVersions());
                _containerHeader = std::make_unique<Objects::FIoContainerHeader>(Ar);
            }
            catch (const std::exception&)
            {
                if (Game() >= Versions::GAME_UE5_0)
                    throw;
                // C#: pre-UE5 readers return null on failure.
            }
        }
        return _containerHeader.get();
    }

    bool IoStoreReader::TryResolve(const FIoChunkId& chunkId, FIoOffsetAndLength& outOffsetLength)
    {
        if (TocResource.ChunkPerfectHashSeeds.has_value())
        {
            const uint32_t chunkCount = TocResource.Header->TocEntryCount;
            if (chunkCount == 0)
            {
                outOffsetLength = {};
                return false;
            }
            const auto seedCount = static_cast<uint32_t>(TocResource.ChunkPerfectHashSeeds->size());
            const auto seedIndex = static_cast<uint32_t>(chunkId.HashWithSeed(0) % seedCount);
            const int32_t seed = (*TocResource.ChunkPerfectHashSeeds)[seedIndex];
            if (seed == 0)
            {
                outOffsetLength = {};
                return false;
            }
            uint32_t slot;
            if (seed < 0)
            {
                const auto seedAsIndex = static_cast<uint32_t>(-seed - 1);
                if (seedAsIndex < chunkCount)
                {
                    slot = seedAsIndex;
                }
                else
                {
                    // Entry without perfect hash
                    return TryResolveImperfect(chunkId, outOffsetLength);
                }
            }
            else
            {
                slot = static_cast<uint32_t>(chunkId.HashWithSeed(seed) % chunkCount);
            }
            // C# compares GetHashCode()s here rather than full equality; kept verbatim.
            if (TocResource.ChunkIds[slot].HashCode() == chunkId.HashCode())
            {
                outOffsetLength = TocResource.ChunkOffsetLengths[slot];
                return true;
            }
            outOffsetLength = {};
            return false;
        }

        return TryResolveImperfect(chunkId, outOffsetLength);
    }

    bool IoStoreReader::TryResolveImperfect(const FIoChunkId& chunkId, FIoOffsetAndLength& outOffsetLength)
    {
        if (TocImperfectHashMapFallback.has_value())
        {
            const auto it = TocImperfectHashMapFallback->find(chunkId);
            if (it == TocImperfectHashMapFallback->end())
            {
                outOffsetLength = {};
                return false;
            }
            outOffsetLength = it->second;
            return true;
        }

        const auto it = std::find(TocResource.ChunkIds.begin(), TocResource.ChunkIds.end(), chunkId);
        if (it == TocResource.ChunkIds.end())
        {
            outOffsetLength = {};
            return false;
        }

        outOffsetLength = TocResource.ChunkOffsetLengths[it - TocResource.ChunkIds.begin()];
        return true;
    }

    std::vector<uint8_t> IoStoreReader::Read(const FIoChunkId& chunkId)
    {
        FIoOffsetAndLength offsetLength;
        if (TryResolve(chunkId, offsetLength))
        {
            return Read(static_cast<int64_t>(offsetLength.Offset()), static_cast<int64_t>(offsetLength.Length()));
        }

        throw std::out_of_range("Couldn't find chunk " + chunkId.ToString() + " in IoStore " + Name());
    }

    std::vector<uint8_t> IoStoreReader::Extract(VirtualFileSystem::VfsEntry& entry,
                                                const Assets::Objects::FByteBulkDataHeader* header)
    {
        auto* ioEntry = dynamic_cast<FIoStoreEntry*>(&entry);
        if (ioEntry == nullptr || entry.Vfs != this)
            throw std::invalid_argument("Wrong io store reader, required " + entry.Vfs->Path() + ", this is " + Path());

        const int64_t offset = ioEntry->Offset;
        int64_t size = ioEntry->Size;
        int64_t offsetInFile = 0;
        if (header != nullptr)
        {
            size = header->SizeOnDisk;
            offsetInFile = header->OffsetInFile;
        }

        return Read(offset, size, offsetInFile);
    }

    std::vector<uint8_t> IoStoreReader::Read(int64_t offset, int64_t length, int64_t offsetInFile)
    {
        if (Game() == GAME_MindsEye)
            return ReadPartiallyEncrypted(offset, length, offsetInFile);

        offset += offsetInFile;
        const uint32_t compressionBlockSize = TocResource.Header->CompressionBlockSize;
        std::vector<uint8_t> dst(static_cast<size_t>(length));
        const auto firstBlockIndex = static_cast<int>(offset / compressionBlockSize);
        const auto lastBlockIndex = static_cast<int>(
            (Utils::Align(offset + static_cast<int64_t>(dst.size()), static_cast<int32_t>(compressionBlockSize)) - 1) /
            compressionBlockSize);
        int64_t offsetInBlock = offset % compressionBlockSize;
        int64_t remainingSize = length;
        int dstOffset = 0;

        std::vector<uint8_t> compressedBuffer;
        std::vector<uint8_t> uncompressedBuffer;
        std::vector<std::unique_ptr<Readers::FArchive>> clonedReaders(ContainerStreams.size());

        for (int blockIndex = firstBlockIndex; blockIndex <= lastBlockIndex; blockIndex++)
        {
            const auto& compressionBlock = TocResource.CompressionBlocks[blockIndex];

            if (Game() == GAME_eBaseballProSpirit)
                throw Exceptions::ParserException(
                    "eBaseballProSpirit io store extraction requires the unported CUE4Parse.GameTypes layer");
            const auto rawSize = Utils::Align(static_cast<int32_t>(compressionBlock.CompressedSize()), Aes::ALIGN);

            if (compressedBuffer.size() < static_cast<size_t>(rawSize))
                compressedBuffer.resize(static_cast<size_t>(rawSize));

            const auto partitionIndex = static_cast<int>(static_cast<uint64_t>(compressionBlock.Offset()) / TocResource.Header->PartitionSize);
            const auto partitionOffset = static_cast<int64_t>(static_cast<uint64_t>(compressionBlock.Offset()) % TocResource.Header->PartitionSize);
            Readers::FArchive* reader;
            if (IsConcurrent())
            {
                auto& clone = clonedReaders[partitionIndex];
                if (clone == nullptr) clone = ContainerStreams[partitionIndex]->Clone();
                reader = clone.get();
            }
            else reader = ContainerStreams[partitionIndex].get();

            reader->ReadAt(partitionOffset, compressedBuffer.data(), 0, static_cast<int>(rawSize));
            // FragPunk decided to encrypt the global utoc too.
            // For Lord of Mysteries utoc files are "synthetic", without dir index, so we can't test the key.
            compressedBuffer = DecryptIfEncrypted(compressedBuffer, 0, static_cast<int>(rawSize), IsEncrypted(),
                Game() == GAME_LordOfMysteries ||
                (Game() == GAME_FragPunk && Path().find("global") != std::string::npos));

            const std::vector<uint8_t>* src;
            if (compressionBlock.CompressionMethodIndex() == 0)
            {
                src = &compressedBuffer;
            }
            else
            {
                const uint32_t uncompressedSize = compressionBlock.UncompressedSize();
                if (uncompressedBuffer.size() < uncompressedSize)
                    uncompressedBuffer.resize(uncompressedSize);

                const auto compressionMethod = TocResource.CompressionMethods[compressionBlock.CompressionMethodIndex()];
                Compression::Compression::Decompress(compressedBuffer, 0, static_cast<int>(compressionBlock.CompressedSize()),
                                                     uncompressedBuffer, 0, static_cast<int>(uncompressedSize),
                                                     compressionMethod, reader);
                src = &uncompressedBuffer;
            }

            const auto sizeInBlock = static_cast<int>(std::min<int64_t>(compressionBlockSize - offsetInBlock, remainingSize));
            std::memcpy(dst.data() + dstOffset, src->data() + offsetInBlock, static_cast<size_t>(sizeInBlock));
            offsetInBlock = 0;
            remainingSize -= sizeInBlock;
            dstOffset += sizeInBlock;
        }

        return dst;
    }

    std::vector<uint8_t> IoStoreReader::ReadPartiallyEncrypted(int64_t offset, int64_t length, int64_t offsetInFile)
    {
        // Only MindsEye reaches this (see Read); its first 0x1000 bytes per chunk are encrypted.
        int limit = 0x1000;

        const uint32_t compressionBlockSize = TocResource.Header->CompressionBlockSize;
        auto firstBlockIndex = static_cast<int>(offset / compressionBlockSize);
        const auto newFirstBlockIndex = static_cast<int>((offset + offsetInFile) / compressionBlockSize);
        if (newFirstBlockIndex != firstBlockIndex)
        {
            limit = 0;
            offset += offsetInFile;
            offsetInFile = 0;
            firstBlockIndex = static_cast<int>(offset / compressionBlockSize);
        }
        else
        {
            length += offsetInFile;
        }

        std::vector<uint8_t> dst(static_cast<size_t>(length));
        const auto lastBlockIndex = static_cast<int>(
            (Utils::Align(offset + static_cast<int64_t>(dst.size()), static_cast<int32_t>(compressionBlockSize)) - 1) /
            compressionBlockSize);
        int64_t offsetInBlock = offset % compressionBlockSize;
        int64_t remainingSize = length;
        int dstOffset = 0;

        std::vector<uint8_t> compressedBuffer;
        std::vector<uint8_t> uncompressedBuffer;
        std::vector<std::unique_ptr<Readers::FArchive>> clonedReaders(ContainerStreams.size());

        for (int blockIndex = firstBlockIndex; blockIndex <= lastBlockIndex; blockIndex++)
        {
            const auto& compressionBlock = TocResource.CompressionBlocks[blockIndex];

            const auto rawSize = Utils::Align(static_cast<int32_t>(compressionBlock.CompressedSize()), Aes::ALIGN);
            if (compressedBuffer.size() < static_cast<size_t>(rawSize))
                compressedBuffer.resize(static_cast<size_t>(rawSize));

            const auto partitionIndex = static_cast<int>(static_cast<uint64_t>(compressionBlock.Offset()) / TocResource.Header->PartitionSize);
            const auto partitionOffset = static_cast<int64_t>(static_cast<uint64_t>(compressionBlock.Offset()) % TocResource.Header->PartitionSize);
            Readers::FArchive* reader;
            if (IsConcurrent())
            {
                auto& clone = clonedReaders[partitionIndex];
                if (clone == nullptr) clone = ContainerStreams[partitionIndex]->Clone();
                reader = clone.get();
            }
            else reader = ContainerStreams[partitionIndex].get();

            reader->ReadAt(partitionOffset, compressedBuffer.data(), 0, static_cast<int>(rawSize));
            if (IsEncrypted() && limit > 0)
            {
                if (rawSize < limit)
                {
                    compressedBuffer = DecryptIfEncrypted(compressedBuffer, 0, static_cast<int>(rawSize), IsEncrypted());
                    limit -= static_cast<int>(rawSize);
                }
                else
                {
                    const auto decrypted = DecryptIfEncrypted(compressedBuffer, 0, limit, IsEncrypted());
                    std::memcpy(compressedBuffer.data(), decrypted.data(), static_cast<size_t>(limit));
                    limit = 0;
                }
            }

            const std::vector<uint8_t>* src;
            if (compressionBlock.CompressionMethodIndex() == 0)
            {
                src = &compressedBuffer;
            }
            else
            {
                const uint32_t uncompressedSize = compressionBlock.UncompressedSize();
                if (uncompressedBuffer.size() < uncompressedSize)
                    uncompressedBuffer.resize(uncompressedSize);

                const auto compressionMethod = TocResource.CompressionMethods[compressionBlock.CompressionMethodIndex()];
                Compression::Compression::Decompress(compressedBuffer, 0, static_cast<int>(compressionBlock.CompressedSize()),
                                                     uncompressedBuffer, 0, static_cast<int>(uncompressedSize),
                                                     compressionMethod, reader);
                src = &uncompressedBuffer;
            }

            const auto sizeInBlock = static_cast<int>(std::min<int64_t>(compressionBlockSize - offsetInBlock, remainingSize));
            std::memcpy(dst.data() + dstOffset, src->data() + offsetInBlock, static_cast<size_t>(sizeInBlock));
            offsetInBlock = 0;
            remainingSize -= sizeInBlock;
            dstOffset += sizeInBlock;
        }

        if (offsetInFile == 0) return dst;
        return std::vector<uint8_t>(dst.begin() + offsetInFile, dst.end());
    }

    void IoStoreReader::Mount(const Utils::StringComparer& pathComparer)
    {
        ProcessIndex(pathComparer);
        // C#'s InitializeContainerHeader: re-arm the lazy container header so it is (re)read on demand.
        _containerHeader.reset();
        _containerHeaderRead = false;
        // C#'s Stopwatch/Serilog mount report is dropped (no logging layer).
    }

    void IoStoreReader::ProcessIndex(const Utils::StringComparer& pathComparer)
    {
        const auto indexBuffer = TocResource.GetDirectoryIndexBuffer();
        if (!HasDirectoryIndex() || !indexBuffer.has_value())
            throw Exceptions::ParserException("No directory index");

        Readers::FByteArchive directoryIndex(Name(), DecryptIfEncrypted(*indexBuffer, IsEncrypted(), true));

        std::string mountPoint;
        try
        {
            mountPoint = directoryIndex.ReadFString();
        }
        catch (const std::exception&)
        {
            throw Exceptions::InvalidAesKeyException(
                "Given aes key '" + (AesKey() != nullptr ? AesKey()->KeyString() : std::string("<null>")) +
                "' is not working with '" + Path() + "'");
        }

        ValidateMountPoint(mountPoint);
        _mountPoint = mountPoint;

        const auto directoryEntries = directoryIndex.ReadArrayCounted<Objects::FIoDirectoryIndexEntry>();
        const auto fileEntries = directoryIndex.ReadArrayCounted<Objects::FIoFileIndexEntry>();
        const int32_t stringCount = directoryIndex.Read<int32_t>();
        std::vector<std::string> stringTable;
        stringTable.reserve(static_cast<size_t>(stringCount));
        for (int32_t i = 0; i < stringCount; ++i)
            stringTable.push_back(directoryIndex.ReadFString());

        GameFileMap files{pathComparer};

        constexpr uint32_t invalidHandle = UINT32_MAX;
        // C# builds paths in a pooled char[]; a by-value std::string per recursion level is the same walk.
        const std::function<void(std::string, uint32_t)> readIndex =
            [&](std::string directoryName, uint32_t dir)
        {
            while (dir != invalidHandle)
            {
                const auto& dirEntry = directoryEntries[dir];
                std::string childDirectoryName = directoryName;
                if (dirEntry.Name != invalidHandle && !stringTable[dirEntry.Name].empty())
                    childDirectoryName += stringTable[dirEntry.Name] + "/";

                uint32_t file = dirEntry.FirstFileEntry;
                while (file != invalidHandle)
                {
                    const auto& fileEntry = fileEntries[file];
                    std::string path = childDirectoryName + stringTable[fileEntry.Name];
                    if (Game() == GAME_NeedForSpeedMobile)
                        path = Utils::SubstringAfter(path, "../../../");

                    auto entry = std::make_shared<FIoStoreEntry>(this, path, fileEntry.UserData);
                    if (entry->IsEncrypted()) _encryptedFileCount++;
                    if (entry->IsUePackage()) PackageIdIndex[entry->ChunkId().AsPackageId()] = entry;
                    files[entry->Path()] = std::move(entry);

                    file = fileEntry.NextFileEntry;
                }

                readIndex(childDirectoryName, dirEntry.FirstChildEntry);
                dir = dirEntry.NextSiblingEntry;
            }
        };
        readIndex(MountPoint(), 0u);

        _files = std::move(files);
    }

    std::vector<uint8_t> IoStoreReader::MountPointCheckBytes()
    {
        auto buffer = TocResource.GetDirectoryIndexBuffer();
        return buffer.has_value() ? std::move(*buffer)
                                  : std::vector<uint8_t>(MAX_MOUNTPOINT_TEST_LENGTH);
    }

    std::vector<uint8_t> IoStoreReader::ReadAndDecrypt(int)
    {
        throw std::logic_error("IoStore can't read bytes without context");
    }
}
