#include "FIoStoreTocResource.h"

#include <cstring>
#include <string>

#include "../../Readers/FByteArchive.h"
#include "../../Objects/Core/Misc/FSHAHash.h"
#include "../../Versions/EGame.h"
#include "../../../Encryption/Aes/Aes.h"
#include "../../../Encryption/Aes/FAesKey.h"
#include "../../../Utils/AlignUtils.h"
#include "../../../Utils/StringUtils.h"

namespace CUE4Parse::UE4::IO::Objects
{
    using namespace CUE4Parse::UE4::Versions;
    using CUE4Parse::Compression::CompressionMethod;
    using CUE4Parse::Encryption::Aes::Aes;
    using CUE4Parse::Encryption::Aes::FAesKey;

    namespace
    {
        bool EndsWith(const std::string& s, const std::string& suffix)
        {
            return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
        }
    }

    FIoStoreTocResource::FIoStoreTocResource(std::shared_ptr<Readers::FArchive> Ar, EIoStoreTocReadOptions readOptions)
        : _tocAr(std::move(Ar))
    {
        std::vector<uint8_t> streamBuffer = _tocAr->ReadBytesAt(0, static_cast<int>(_tocAr->Length));

        if (_tocAr->Game() == GAME_TheFinals || _tocAr->Game() == GAME_ArcRaiders)
        {
            // Self-contained obfuscation: everything after the header is AES'd with a hardcoded key.
            const FAesKey key("0x5A4741BC469E10E569D48057B7AB43320388C9748759663BB5D13E201CA2052E");
            const auto decrypted = Aes::Decrypt(streamBuffer, FIoStoreTocHeader::SIZE,
                                                static_cast<int>(_tocAr->Length) - FIoStoreTocHeader::SIZE, key);
            std::memcpy(streamBuffer.data() + FIoStoreTocHeader::SIZE, decrypted.data(), decrypted.size());
        }

        Readers::FByteArchive archive(_tocAr->Name(), std::move(streamBuffer), _tocAr->Versions);

        // Header
        Header = std::make_unique<FIoStoreTocHeader>(archive);

        if (Header->Version < EIoStoreTocVersion::PartitionSize)
        {
            Header->PartitionCount = 1;
            Header->PartitionSize = UINT64_MAX;
        }

        // Chunk IDs
        ChunkIds = archive.ReadArray<FIoChunkId>(static_cast<int>(Header->TocEntryCount));

        // Chunk offsets
        ChunkOffsetLengths = archive.ReadArray<FIoOffsetAndLength>(static_cast<int>(Header->TocEntryCount));

        if (_tocAr->Game() == GAME_NeedForSpeedMobile && !EndsWith(_tocAr->Name(), "global.utoc"))
        {
            // Also self-contained: the offset/length table is AES'd with a hardcoded key.
            archive.Position -= static_cast<int64_t>(Header->TocEntryCount) * 10;
            const int len = Utils::Align(static_cast<int32_t>(Header->TocEntryCount) * 10, 16);
            const FAesKey key("0xB71C91417A3790F27BE3852C6775EBF39D88BEABC0CDDCF721F7B2F0CA69FA12");
            const auto data = Aes::Decrypt(archive.ReadBytes(len), key);
            Readers::FByteArchive chunksAr("ChunkOffsetLengths", data);
            ChunkOffsetLengths = chunksAr.ReadArray<FIoOffsetAndLength>(static_cast<int>(Header->TocEntryCount));
        }

        // Chunk perfect hash map
        uint32_t perfectHashSeedsCount = 0;
        uint32_t chunksWithoutPerfectHashCount = 0;
        if (Header->Version >= EIoStoreTocVersion::PerfectHashWithOverflow)
        {
            perfectHashSeedsCount = Header->TocChunkPerfectHashSeedsCount;
            chunksWithoutPerfectHashCount = Header->TocChunksWithoutPerfectHashCount;
        }
        else if (Header->Version >= EIoStoreTocVersion::PerfectHash)
        {
            perfectHashSeedsCount = Header->TocChunkPerfectHashSeedsCount;
        }
        if (perfectHashSeedsCount > 0)
        {
            ChunkPerfectHashSeeds = archive.ReadArray<int32_t>(static_cast<int>(perfectHashSeedsCount));
        }
        if (chunksWithoutPerfectHashCount > 0)
        {
            ChunkIndicesWithoutPerfectHash = archive.ReadArray<int32_t>(static_cast<int>(chunksWithoutPerfectHashCount));
        }

        // Compression blocks
        const bool isFragPunk = archive.Game() == GAME_FragPunk;
        CompressionBlocks.reserve(Header->TocCompressedBlockEntryCount);
        for (uint32_t i = 0; i < Header->TocCompressedBlockEntryCount; i++)
        {
            CompressionBlocks.emplace_back(archive);
            if (isFragPunk) archive.Position += 4;
        }

        // Compression methods
        {
            const int bufferSize = static_cast<int>(Header->CompressionMethodNameLength * Header->CompressionMethodNameCount);
            const std::vector<uint8_t> buffer = archive.ReadBytes(bufferSize);
            CompressionMethods.assign(Header->CompressionMethodNameCount + 1, CompressionMethod::None);
            for (uint32_t i = 0; i < Header->CompressionMethodNameCount; i++)
            {
                const char* start = reinterpret_cast<const char*>(buffer.data()) + i * Header->CompressionMethodNameLength;
                std::string name(start, Header->CompressionMethodNameLength);
                while (!name.empty() && name.back() == '\0') name.pop_back();
                if (name.empty())
                    continue;
                CompressionMethod method;
                if (!Compression::TryParseCompressionMethod(name, method, /*ignoreCase*/ true))
                {
                    // C# logs a warning for an unknown method; the port has no logging layer.
                    method = CompressionMethod::Unknown;
                }
                CompressionMethods[i + 1] = method;
            }
        }

        // Chunk block signatures
        if (HasFlag(Header->ContainerFlags, EIoContainerFlags::Signed))
        {
            const int32_t hashSize = archive.Read<int32_t>();
            // tocSignature and blockSignature both byte[hashSize], then FSHAHash[TocCompressedBlockEntryCount].
            archive.Position += static_cast<int64_t>(hashSize) + hashSize +
                                static_cast<int64_t>(UE4::Objects::Core::Misc::FSHAHash::SIZE) * Header->TocCompressedBlockEntryCount;
            // You could verify hashes here but nah
        }

        // Directory index
        if (Header->Version >= EIoStoreTocVersion::DirectoryIndex &&
            HasFlag(Header->ContainerFlags, EIoContainerFlags::Indexed) &&
            Header->DirectoryIndexSize > 0)
        {
            if (HasFlag(readOptions, EIoStoreTocReadOptions::ReadDirectoryIndex))
            {
                DirectoryIndexBufferOffset = archive.Position;
            }
            else
            {
                archive.Position += Header->DirectoryIndexSize;
            }
        }

        // Meta
        if (HasFlag(readOptions, EIoStoreTocReadOptions::ReadTocMeta))
        {
            const bool replacedIoChunkHashWithIoHash = Header->Version >= EIoStoreTocVersion::ReplaceIoChunkHashWithIoHash;
            ChunkMetas.emplace();
            ChunkMetas->reserve(Header->TocEntryCount);
            for (uint32_t i = 0; i < Header->TocEntryCount; i++)
            {
                ChunkMetas->emplace_back(archive, replacedIoChunkHashWithIoHash);
            }
            // The OnDemand hash tables (Version == OnDemandMetaData) are skipped by C# too; nothing here
            // reads past them, so the skip is omitted along with the on-demand layer.
        }
    }

    std::optional<std::vector<uint8_t>> FIoStoreTocResource::GetDirectoryIndexBuffer() const
    {
        if (_tocAr == nullptr || DirectoryIndexBufferOffset == -1)
            return std::nullopt;

        if (_tocAr->Game() == GAME_TheFinals || _tocAr->Game() == GAME_ArcRaiders)
        {
            const int64_t readOffset = DirectoryIndexBufferOffset & ~(static_cast<int64_t>(Aes::ALIGN) - 1);
            const int64_t dataOffset = DirectoryIndexBufferOffset - readOffset;
            const int64_t readSize = Utils::Align(dataOffset + Header->DirectoryIndexSize, Aes::ALIGN);
            const FAesKey key("0x5A4741BC469E10E569D48057B7AB43320388C9748759663BB5D13E201CA2052E");
            auto decrypted = Aes::Decrypt(_tocAr->ReadBytesAt(readOffset, static_cast<int>(readSize)), key);
            if (dataOffset == 0 && Header->DirectoryIndexSize == decrypted.size())
                return decrypted;
            return std::vector<uint8_t>(decrypted.begin() + dataOffset,
                                        decrypted.begin() + dataOffset + Header->DirectoryIndexSize);
        }

        return _tocAr->ReadBytesAt(DirectoryIndexBufferOffset, static_cast<int>(Header->DirectoryIndexSize));
    }
}
