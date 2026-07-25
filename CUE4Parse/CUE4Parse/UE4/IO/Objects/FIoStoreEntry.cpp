#include "FIoStoreEntry.h"

#include <cstdio>

#include "../IoStoreReader.h"
#include "../../Readers/FByteArchive.h"

namespace CUE4Parse::UE4::IO::Objects
{
    FIoStoreEntry::FIoStoreEntry(IoStoreReader* reader, std::string path, uint32_t tocEntryIndex)
        : VfsEntry(reader, std::move(path)), _tocEntryIndex(tocEntryIndex)
    {
        const auto& offsetLength = reader->TocResource.ChunkOffsetLengths[tocEntryIndex];
        Offset = static_cast<int64_t>(offsetLength.Offset());
        Size = static_cast<int64_t>(offsetLength.Length());
    }

    FIoStoreEntry::FIoStoreEntry(IoStoreReader* reader, uint32_t tocEntryIndex)
        : VfsEntry(reader, "NonIndexed/"), _tocEntryIndex(tocEntryIndex)
    {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "0x%08llX", static_cast<unsigned long long>(ChunkId().ChunkId));
        SetPath(Path() + buf + "." + ChunkId().GetExtension(*reader));

        const auto& offsetLength = reader->TocResource.ChunkOffsetLengths[tocEntryIndex];
        Offset = static_cast<int64_t>(offsetLength.Offset());
        Size = static_cast<int64_t>(offsetLength.Length());
    }

    bool FIoStoreEntry::IsEncrypted() const { return GetIoStoreReader().IsEncrypted(); }

    Compression::CompressionMethod FIoStoreEntry::GetCompressionMethod() const
    {
        const auto& tocResource = GetIoStoreReader().TocResource;
        const auto firstBlockIndex = static_cast<int>(Offset / tocResource.Header->CompressionBlockSize);
        return tocResource.CompressionMethods[tocResource.CompressionBlocks[firstBlockIndex].CompressionMethodIndex()];
    }

    const FIoChunkId& FIoStoreEntry::ChunkId() const
    {
        return GetIoStoreReader().TocResource.ChunkIds[_tocEntryIndex];
    }

    IoStoreReader& FIoStoreEntry::GetIoStoreReader() const
    {
        // Virtual base, so the downcast needs dynamic_cast (see FPakEntry::GetPakFileReader).
        return *dynamic_cast<IoStoreReader*>(Vfs);
    }

    std::vector<uint8_t> FIoStoreEntry::Read(const CUE4Parse::UE4::Assets::Objects::FByteBulkDataHeader* header)
    {
        return Vfs->Extract(*this, header);
    }

    std::unique_ptr<Readers::FArchive> FIoStoreEntry::CreateReader(const CUE4Parse::UE4::Assets::Objects::FByteBulkDataHeader* header)
    {
        return std::make_unique<Readers::FByteArchive>(Path(), Read(header), Vfs->GetVersions());
    }
}
