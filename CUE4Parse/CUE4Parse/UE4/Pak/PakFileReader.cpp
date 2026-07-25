#include "PakFileReader.h"

#include "../Assets/Objects/FByteBulkDataHeader.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "../Exceptions/InvalidAesKeyException.h"
#include "../Exceptions/ParserException.h"
#include "../Readers/FByteArchive.h"
#include "../Readers/FMemoryImageArchive.h"
#include "../Readers/FStreamArchive.h"
#include "../../Compression/Compression.h"
#include "../../Encryption/Aes/Aes.h"
#include "../../Utils/AlignUtils.h"

namespace CUE4Parse::UE4::Pak
{
    using namespace CUE4Parse::UE4::Versions;
    using CUE4Parse::Compression::CompressionMethod;
    using CUE4Parse::Encryption::Aes::Aes;
    using Objects::EPakFileVersion;
    using Objects::FPakEntry;

    namespace
    {
        [[noreturn]] void UnportedGameType(const char* what)
        {
            throw Exceptions::ParserException(std::string(what) + " requires the unported CUE4Parse.GameTypes layer");
        }
    }

    PakFileReader::PakFileReader(std::shared_ptr<Readers::FArchive> ar)
        : AbstractAesVfsReader(ar->Name(), ar->Versions), Ar(std::move(ar)),
          Info(Objects::FPakInfo::ReadFPakInfo(*Ar))
    {
        _length = Ar->Length;
        CompressionMethods() = Info.CompressionMethods;

        const bool hasUnsupportedVersion =
            (Ar->Game() < GAME_UE5_7 && Info.Version > EPakFileVersion::PakFile_Version_Fnv64BugFix) ||
            (Ar->Game() >= GAME_UE5_7 && Info.Version > EPakFileVersion::PakFile_Version_Latest);
        // C# logs a warning for an unsupported version here; the port has no logging layer.
        (void) (hasUnsupportedVersion && !UsingCustomPakVersion());
    }

    PakFileReader::PakFileReader(const std::string& filePath, Versions::VersionContainer versions)
        : PakFileReader(std::make_shared<Readers::FRandomAccessFileStreamArchive>(filePath, std::move(versions))) {}

    bool PakFileReader::UsingCustomPakVersion() const
    {
        switch (Ar->Game())
        {
            case GAME_InfinityNikki: case GAME_MeetYourMaker: case GAME_DeadByDaylight: case GAME_WutheringWaves:
            case GAME_Snowbreak: case GAME_TorchlightInfinite: case GAME_TowerOfFantasy:
            case GAME_TheDivisionResurgence: case GAME_QQ: case GAME_DreamStar:
            case GAME_EtheriaRestart: case GAME_DeadByDaylight_Old: case GAME_WorldofJadeDynasty:
            case GAME_EmbersofTheUncrowned: case GAME_ValorantSource:
                return true;
            default:
                return false;
        }
    }

    std::vector<uint8_t> PakFileReader::ReadAndDecrypt(int length)
    {
        return AbstractAesVfsReader::ReadAndDecrypt(length, *Ar, IsEncrypted());
    }

    std::vector<uint8_t> PakFileReader::ReadAndDecryptIndex(int length)
    {
        return AbstractAesVfsReader::ReadAndDecryptIndex(length, *Ar, IsEncrypted());
    }

    std::vector<uint8_t> PakFileReader::MountPointCheckBytes()
    {
        std::unique_ptr<Readers::FArchive> clone;
        Readers::FArchive* reader = Ar.get();
        if (IsConcurrent()) { clone = Ar->Clone(); reader = clone.get(); }

        reader->Position = Info.IndexOffset;
        const int size = static_cast<int>(std::min<int64_t>(Info.IndexSize, 4 + MAX_MOUNTPOINT_TEST_LENGTH * 2));
        return reader->ReadBytes(static_cast<int>(Utils::Align(size, Aes::ALIGN)));
    }

    std::vector<uint8_t> PakFileReader::Extract(VirtualFileSystem::VfsEntry& entry,
                                                const Assets::Objects::FByteBulkDataHeader* header)
    {
        auto* pakEntry = dynamic_cast<FPakEntry*>(&entry);
        if (pakEntry == nullptr || entry.Vfs != static_cast<VirtualFileSystem::IVfsReader*>(this))
            throw std::invalid_argument("Wrong pak file reader, required " + entry.Vfs->Name() + ", this is " + Name());

        // If this reader is used as a concurrent reader create a clone of the main reader to provide thread safety
        std::unique_ptr<Readers::FArchive> clone;
        Readers::FArchive* reader = Ar.get();
        if (IsConcurrent()) { clone = Ar->Clone(); reader = clone.get(); }

        const int alignment = pakEntry->IsEncrypted() ? Aes::ALIGN : 1;

        // With a header the caller wants one bulk sub-range of the entry rather than all of it; the
        // block arithmetic below is written against these two and so covers both cases.
        int64_t offset = 0;
        int requestedSize = static_cast<int>(pakEntry->UncompressedSize);
        if (header != nullptr)
        {
            offset = header->OffsetInFile;
            requestedSize = static_cast<int>(header->SizeOnDisk);
        }

        if (pakEntry->IsCompressed())
        {
            switch (Game())
            {
                case GAME_MarvelRivals: case GAME_OperationApocalypse: case GAME_WutheringWaves: case GAME_MindsEye:
                    UnportedGameType("Partially-encrypted compressed extraction");
                case GAME_GameForPeace: UnportedGameType("GameForPeace extraction");
                case GAME_Rennsport: UnportedGameType("Rennsport extraction");
                case GAME_DragonQuestXI: UnportedGameType("Dragon Quest XI extraction");
                case GAME_CenturyAgeofAshes:
                    if (pakEntry->CompressionMethod == CompressionMethod::PWC) UnportedGameType("Century: Age of Ashes extraction");
                    break;
                case GAME_ArenaBreakoutInfinite: case GAME_ArenaBreakoutMobile: UnportedGameType("Arena Breakout extraction");
                case GAME_eBaseballProSpirit: UnportedGameType("eBaseball Pro Spirit extraction");
                default: break;
            }

            const int compressionBlockSize = static_cast<int>(pakEntry->CompressionBlockSize);
            const int64_t firstBlockIndex = offset / compressionBlockSize;
            const int64_t lastBlockIndex = (offset + requestedSize - 1) / compressionBlockSize;

            // blocks are full size, except potentially the last one
            const int64_t numBlocks = lastBlockIndex - firstBlockIndex + 1;
            int64_t bufferSize = numBlocks * compressionBlockSize;
            if (lastBlockIndex == static_cast<int>((pakEntry->UncompressedSize - 1) / compressionBlockSize))
            {
                const int lastBlockInFileSize = static_cast<int>(pakEntry->UncompressedSize % compressionBlockSize);
                if (lastBlockInFileSize > 0)
                    bufferSize -= compressionBlockSize - lastBlockInFileSize;
            }

            std::vector<uint8_t> uncompressed(static_cast<size_t>(bufferSize));
            int uncompressedOff = 0;

            std::vector<uint8_t> compressedBuffer;
            // decompress the required blocks
            for (int64_t blockIndex = firstBlockIndex; blockIndex <= lastBlockIndex; blockIndex++)
            {
                const Objects::FPakCompressedBlock& block = pakEntry->CompressionBlocks[static_cast<size_t>(blockIndex)];
                const int blockSize = static_cast<int>(block.Size());
                const int srcSize = static_cast<int>(Utils::Align(blockSize, alignment));
                if (static_cast<size_t>(srcSize) > compressedBuffer.size())
                    compressedBuffer.assign(static_cast<size_t>(srcSize), 0);
                // Read the compressed block
                const std::vector<uint8_t> compressed =
                    ReadAndDecryptAt(compressedBuffer, block.CompressedStart, srcSize, *reader, pakEntry->IsEncrypted());
                // Calculate the uncompressed size,
                // its either just the compression block size,
                // or if it's the last block, it's the remaining data size
                const int uncompressedSize = static_cast<int>(std::min<int64_t>(
                    compressionBlockSize, pakEntry->UncompressedSize - blockIndex * compressionBlockSize));
                Compression::Compression::Decompress(compressed, 0, blockSize, uncompressed, uncompressedOff,
                                                     uncompressedSize, pakEntry->CompressionMethod, reader);
                uncompressedOff += uncompressedSize;
            }

            // C# runs a per-game post-decryption pass over `uncompressed` here (Lua/ini/csv); not ported.

            const int64_t offsetInFirstBlock = offset - firstBlockIndex * compressionBlockSize;
            if (offsetInFirstBlock == 0 && requestedSize == bufferSize)
                return uncompressed;

            std::vector<uint8_t> result(static_cast<size_t>(requestedSize));
            std::copy_n(uncompressed.begin() + offsetInFirstBlock, requestedSize, result.begin());
            return result;
        }

        switch (Game())
        {
            case GAME_MarvelRivals: case GAME_OperationApocalypse: case GAME_WutheringWaves: case GAME_MindsEye:
                UnportedGameType("Partially-encrypted extraction");
            case GAME_Rennsport: UnportedGameType("Rennsport extraction");
            case GAME_DragonQuestXI: UnportedGameType("Dragon Quest XI extraction");
            case GAME_ArenaBreakoutInfinite: case GAME_ArenaBreakoutMobile: UnportedGameType("Arena Breakout extraction");
            case GAME_eBaseballProSpirit: UnportedGameType("eBaseball Pro Spirit extraction");
            default: break;
        }

        // Pak Entry is written before the file data,
        // but it's the same as the one from the index, just without a name
        // We don't need to serialize that again so + file.StructSize

        const int64_t readOffset = offset & ~(static_cast<int64_t>(alignment) - 1);
        const int64_t dataOffset = offset - readOffset;
        const int64_t readSize = Utils::Align(dataOffset + requestedSize, alignment);
        std::vector<uint8_t> data = ReadAndDecryptAt(pakEntry->Offset + pakEntry->StructSize + readOffset,
                                                     static_cast<int>(readSize), *reader, pakEntry->IsEncrypted());

        // C# runs the same per-game post-decryption pass over `data` here; not ported.

        if (dataOffset == 0 && requestedSize == static_cast<int>(data.size()))
            return data;

        std::vector<uint8_t> chunk(static_cast<size_t>(requestedSize));
        std::copy_n(data.begin() + dataOffset, requestedSize, chunk.begin());
        return chunk;
    }

    void PakFileReader::Mount(const Utils::StringComparer& pathComparer)
    {
        if (Info.Version >= EPakFileVersion::PakFile_Version_PathHashIndex)
        {
            switch (Game())
            {
                case GAME_CrystalOfAtlan: UnportedGameType("Crystal of Atlan index");
                case GAME_DragonSwordAwakening: UnportedGameType("Dragon Sword Awakening index");
                case GAME_ValorantSource: UnportedGameType("ValorantSource index");
                default: ReadIndexUpdated(pathComparer); break;
            }
        }
        else if (Info.IndexIsFrozen)
            ReadFrozenIndex(pathComparer);
        else
            ReadIndexLegacy(pathComparer);

        // C# warns when a pak is not encrypted but contains encrypted files, and logs a mount summary here.
    }

    void PakFileReader::ReadIndexLegacy(const Utils::StringComparer& pathComparer)
    {
        Ar->Position = Info.IndexOffset;
        Readers::FByteArchive index(Name() + " - Index", ReadAndDecryptIndex(static_cast<int>(Info.IndexSize)), _versions);

        std::string mountPoint;
        try
        {
            mountPoint = index.ReadFString();
        }
        catch (const std::exception&)
        {
            throw Exceptions::InvalidAesKeyException(
                "Given aes key '" + (AesKey() ? AesKey()->KeyString() : std::string()) + "' is not working with '" + Name() + "'");
        }

        ValidateMountPoint(mountPoint);
        _mountPoint = mountPoint;

        if (Ar->Game() == GAME_GameForPeace) UnportedGameType("GameForPeace index");
        if (Ar->Game() == GAME_DragonQuestXI) UnportedGameType("Dragon Quest XI index");

        int32_t fileCount = index.Read<int32_t>();
        if (Ar->Game() == GAME_TransformersOnline) fileCount -= 100;

        GameFileMap files(pathComparer);
        for (int32_t i = 0; i < fileCount; i++)
        {
            const std::string path = mountPoint + index.ReadFString();
            auto entry = std::make_shared<FPakEntry>(*this, path, index);
            if (entry->IsDeleted() && entry->Size == 0) continue;
            if (entry->IsEncrypted()) _encryptedFileCount++;
            files[path] = entry;
        }

        _files = std::move(files);
    }

    void PakFileReader::ReadIndexUpdated(const Utils::StringComparer& pathComparer)
    {
        // Prepare primary index and decrypt if necessary.
        // Note (faithful to C#): the primary index archive is built WITHOUT a version container, so the
        // FPakEntry constructors that consult `Ar.Game` see the default game rather than this pak's. Every
        // decision that matters is taken off `reader.Info` / `reader.Game` instead.
        Ar->Position = Info.IndexOffset;
        Readers::FByteArchive primaryIndex(Name() + " - Primary Index", ReadAndDecryptIndex(static_cast<int>(Info.IndexSize)));

        int32_t fileCount = 0;
        _encryptedFileCount = 0;

        if (Ar->Game() == GAME_DreamStar || Ar->Game() == GAME_DeltaForce)
        {
            primaryIndex.Position += 8; // PathHashSeed
            fileCount = primaryIndex.Read<int32_t>();
        }

        std::string mountPoint;
        try
        {
            mountPoint = primaryIndex.ReadFString();
        }
        catch (const std::exception&)
        {
            throw Exceptions::InvalidAesKeyException(
                "Given aes key '" + (AesKey() ? AesKey()->KeyString() : std::string()) + "' is not working with '" + Name() + "'");
        }

        ValidateMountPoint(mountPoint);
        _mountPoint = mountPoint;

        if (!(Ar->Game() == GAME_DreamStar || Ar->Game() == GAME_DeltaForce))
        {
            fileCount = primaryIndex.Read<int32_t>();
            primaryIndex.Position += 8; // PathHashSeed
        }

        if (!primaryIndex.ReadBoolean())
            throw Exceptions::ParserException(primaryIndex, "No path hash index");

        primaryIndex.Position += 36; // PathHashIndexOffset (long) + PathHashIndexSize (long) + PathHashIndexHash (20 bytes)
        if (Ar->Game() == GAME_Rennsport) primaryIndex.Position += 16;

        if (!primaryIndex.ReadBoolean())
            throw Exceptions::ParserException(primaryIndex, "No directory index");

        if (Ar->Game() == GAME_TheDivisionResurgence) primaryIndex.Position += 40; // duplicate entry

        const int64_t directoryIndexOffset = primaryIndex.Read<int64_t>();
        const int64_t directoryIndexSize = primaryIndex.Read<int64_t>();
        primaryIndex.Position += 20; // Directory Index hash
        if (Ar->Game() == GAME_Rennsport) primaryIndex.Position += 20;
        int32_t encodedPakEntriesSize = primaryIndex.Read<int32_t>();
        if (Ar->Game() == GAME_Rennsport)
        {
            primaryIndex.Position -= 4;
            encodedPakEntriesSize = static_cast<int32_t>(primaryIndex.Length - primaryIndex.Position - 6);
        }

        Readers::FByteArchive encodedPakEntries("Encoded Pak Entries", primaryIndex.ReadBytes(encodedPakEntriesSize));

        const int32_t filesNum = primaryIndex.Read<int32_t>();
        if (filesNum < 0)
            throw Exceptions::ParserException("Corrupt pak PrimaryIndex detected");

        std::vector<std::shared_ptr<FPakEntry>> nonEncodedEntries;
        nonEncodedEntries.reserve(static_cast<size_t>(filesNum));
        for (int32_t i = 0; i < filesNum; i++)
            nonEncodedEntries.push_back(std::make_shared<FPakEntry>(*this, std::string(), primaryIndex));

        // Read FDirectoryIndex
        Ar->Position = directoryIndexOffset;
        if (Ar->Game() == GAME_Rennsport) UnportedGameType("Rennsport directory index");
        Readers::FByteArchive directoryIndex("Directory Index", ReadAndDecryptIndex(static_cast<int>(directoryIndexSize)));

        GameFileMap files(pathComparer);

        if (Info.Version >= EPakFileVersion::PakFile_Version_SortedDirectoryIndex && Ar->Game() >= GAME_UE5_9)
        {
            ReadFlatDirectoryIndex(directoryIndex, files, encodedPakEntries, nonEncodedEntries);
            _files = std::move(files);
            return;
        }

        const int32_t directoryIndexLength = directoryIndex.Read<int32_t>();
        for (int32_t dirIndex = 0; dirIndex < directoryIndexLength; dirIndex++)
        {
            std::string dir = directoryIndex.ReadFString();
            const bool trimDir = !_mountPoint.empty() && !dir.empty() && dir[0] == '/' && _mountPoint.back() == '/';
            if (trimDir) dir.erase(0, 1);

            const int32_t fileEntries = directoryIndex.Read<int32_t>();
            for (int32_t fileIndex = 0; fileIndex < fileEntries; fileIndex++)
            {
                // supports PakFile_Version_Utf8PakDirectory too (ReadFString handles both encodings)
                const std::string fileName = directoryIndex.ReadFString();
                const std::string path = _mountPoint + dir + fileName;

                const int32_t offset = directoryIndex.Read<int32_t>();
                if (offset == INT32_MIN) continue;

                std::shared_ptr<FPakEntry> entry;
                if (offset >= 0)
                {
                    entry = std::make_shared<FPakEntry>(*this, path, encodedPakEntries, offset);
                }
                else
                {
                    const int32_t index = -offset - 1;
                    // C# logs a warning and skips on an out-of-range index; same behaviour, minus the log.
                    if (index < 0 || static_cast<size_t>(index) >= nonEncodedEntries.size()) continue;

                    entry = nonEncodedEntries[static_cast<size_t>(index)];
                    entry->SetPath(path);
                }
                if (entry->IsEncrypted()) _encryptedFileCount++;
                files[path] = entry;
            }
        }

        _files = std::move(files);
    }

    void PakFileReader::ReadFlatDirectoryIndex(Readers::FArchive& directoryIndex, GameFileMap& files,
                                               Readers::FArchive& encodedPakEntries,
                                               std::vector<std::shared_ptr<FPakEntry>>& nonEncodedEntries)
    {
        constexpr int32_t flatMagic = 0x50464451; // 'PFDQ'
        if (directoryIndex.Read<int32_t>() != flatMagic)
            throw Exceptions::ParserException("Corrupt pak FullDirectoryIndex (flat) detected");

        const int32_t numDirs = directoryIndex.Read<int32_t>();
        const int32_t numFiles = directoryIndex.Read<int32_t>();
        const int32_t restartInterval = directoryIndex.Read<int32_t>();
        const int32_t dirBlobBytes = directoryIndex.Read<int32_t>();
        const int32_t fileBlobBytes = directoryIndex.Read<int32_t>();
        const int32_t numPathHashes = directoryIndex.Read<int32_t>();
        directoryIndex.Position += sizeof(int32_t); // pad that 8-aligns the following uint64 hash table

        if (numDirs < 0 || numFiles < 0 || restartInterval <= 0 || dirBlobBytes < 0 || fileBlobBytes < 0 || numPathHashes < 0)
            throw Exceptions::ParserException("Corrupt pak FullDirectoryIndex (flat) detected");

        const int32_t numDirAnchors = (numDirs + restartInterval - 1) / restartInterval;

        directoryIndex.Position += static_cast<int64_t>(numPathHashes) * sizeof(uint64_t); // SortedPathHashes
        directoryIndex.Position += static_cast<int64_t>(numPathHashes) * sizeof(int32_t); // HashLocations
        directoryIndex.Position += static_cast<int64_t>(numDirAnchors + 1) * sizeof(int32_t); // DirAnchorOffset

        const std::vector<int32_t> dirFileStart = directoryIndex.ReadArray<int32_t>(numDirs + 1);
        const std::vector<int32_t> fileNameOffsets = directoryIndex.ReadArray<int32_t>(numFiles + 1);
        const std::vector<int32_t> fileLocations = directoryIndex.ReadArray<int32_t>(numFiles);
        const std::vector<uint8_t> dirBlob = directoryIndex.ReadArray<uint8_t>(dirBlobBytes);
        const std::vector<uint8_t> fileBlob = directoryIndex.ReadArray<uint8_t>(fileBlobBytes);
        const bool trimMountSep = !_mountPoint.empty() && _mountPoint.back() == '/';

        int32_t dirPos = 0;
        // Front-coded directory names: each entry keeps a prefix of the previous one and appends a suffix.
        std::string nameBytes;
        for (int32_t dirIndex = 0; dirIndex < numDirs; dirIndex++)
        {
            int32_t sharedLen;
            std::memcpy(&sharedLen, dirBlob.data() + dirPos, sizeof(int32_t));
            dirPos += sizeof(int32_t);
            int32_t suffixLen;
            std::memcpy(&suffixLen, dirBlob.data() + dirPos, sizeof(int32_t));
            dirPos += sizeof(int32_t);

            nameBytes.resize(static_cast<size_t>(sharedLen));
            nameBytes.append(reinterpret_cast<const char*>(dirBlob.data()) + dirPos, static_cast<size_t>(suffixLen));
            dirPos += suffixLen;

            // Mirror ReadIndexUpdated
            const bool trimDir = trimMountSep && !nameBytes.empty() && nameBytes[0] == '/';
            const std::string dir = trimDir ? nameBytes.substr(1) : nameBytes;

            for (int32_t global = dirFileStart[static_cast<size_t>(dirIndex)];
                 global < dirFileStart[static_cast<size_t>(dirIndex) + 1]; global++)
            {
                const int32_t location = fileLocations[static_cast<size_t>(global)];
                if (location == INT32_MIN) continue;

                const int32_t nameStart = fileNameOffsets[static_cast<size_t>(global)];
                const std::string fileName(reinterpret_cast<const char*>(fileBlob.data()) + nameStart,
                                           static_cast<size_t>(fileNameOffsets[static_cast<size_t>(global) + 1] - nameStart));
                const std::string path = _mountPoint + dir + fileName;

                std::shared_ptr<FPakEntry> entry;
                if (location >= 0)
                {
                    entry = std::make_shared<FPakEntry>(*this, path, encodedPakEntries, location);
                }
                else
                {
                    const int32_t entryIndex = -location - 1;
                    if (entryIndex < 0 || static_cast<size_t>(entryIndex) >= nonEncodedEntries.size()) continue;

                    entry = nonEncodedEntries[static_cast<size_t>(entryIndex)];
                    entry->SetPath(path);
                }

                if (entry->IsEncrypted()) _encryptedFileCount++;
                files[path] = entry;
            }
        }
    }

    void PakFileReader::ReadFrozenIndex(const Utils::StringComparer& pathComparer)
    {
        Ar->Position = Info.IndexOffset;
        // As in C#, the inner byte archive carries no version container (see the note in ReadIndexUpdated).
        auto inner = std::make_shared<Readers::FByteArchive>("FPakFileData", Ar->ReadBytes(static_cast<int>(Info.IndexSize)));
        Readers::FMemoryImageArchive mem(inner, 8);

        std::string mountPoint = mem.ReadFString();
        ValidateMountPoint(mountPoint);
        _mountPoint = mountPoint;

        std::vector<std::shared_ptr<FPakEntry>> entries =
            mem.ReadArrayWith([&] { return std::make_shared<FPakEntry>(*this, mem); });

        // read TMap<FString, TMap<FString, int32>>
        auto index = mem.ReadTMap(
            [&] { return mem.ReadFString(); },
            [&] {
                return mem.ReadTMap(
                    [&] { return mem.ReadFString(); },
                    [&] { return mem.Read<int32_t>(); },
                    16, 4);
            },
            16, 56);

        GameFileMap files(pathComparer);
        for (const auto& [dir, dirContents] : index)
        {
            for (const auto& [name, fileIndex] : dirContents)
            {
                std::string path;
                if (!mountPoint.empty() && mountPoint.back() == '/' && !dir.empty() && dir.front() == '/')
                    path = dir.size() == 1 ? mountPoint + name : mountPoint + dir.substr(1) + name;
                else
                    path = mountPoint + dir + name;

                if (fileIndex < 0 || static_cast<size_t>(fileIndex) >= entries.size()) continue;
                auto entry = entries[static_cast<size_t>(fileIndex)];
                entry->SetPath(path);

                if (entry->IsDeleted() && entry->Size == 0) continue;
                if (entry->IsEncrypted()) _encryptedFileCount++;
                files[path] = entry;
            }
        }

        _files = std::move(files);
    }
}
