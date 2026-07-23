// Ported from CUE4Parse/UE4/Pak/Objects/FPakEntry.cs
// One file's record inside a pak: where it starts, how big it is, how it is compressed, and (when it is
// compressed) the byte range of every compression block.
//
// The five constructors correspond to the five index formats: the legacy inline record, the bit-packed
// "encoded" record of the updated index, the frozen memory-image record, and GameForPeace's custom record.
//
// Deliberate differences from C#:
//   * C#'s GenericBufferReader for the encoded-entries blob is just a memory reader; FArchive& covers it.
//   * The ValorantSource and InfinityNikki/WutheringWaves bit-shuffles ARE ported (they are self-contained
//     arithmetic) apart from ValorantSource's offset/size reconstruction, which needs the unported Tencent
//     mask constants — that branch throws, matching FPakInfo.
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "FPakCompressedBlock.h"
#include "FPakInfo.h"
#include "../../VirtualFileSystem/VfsEntry.h"
#include "../../Readers/FArchive.h"
#include "../../Readers/FMemoryImageArchive.h"
#include "../../Versions/EGame.h"

namespace CUE4Parse::UE4::Pak { class PakFileReader; }

namespace CUE4Parse::UE4::Pak::Objects
{
    class FPakEntry : public VirtualFileSystem::VfsEntry
    {
    public:
        static constexpr uint8_t Flag_None = 0x00;
        static constexpr uint8_t Flag_Encrypted = 0x01;
        static constexpr uint8_t Flag_Deleted = 0x02;

        int64_t CompressedSize = 0;
        int64_t UncompressedSize = 0;
        Compression::CompressionMethod CompressionMethod = Compression::CompressionMethod::None;
        std::vector<FPakCompressedBlock> CompressionBlocks;
        uint32_t Flags = 0;
        uint32_t CompressionBlockSize = 0;
        int32_t CustomData = 0;

        int32_t StructSize = 0; // computed value: size of FPakEntry prepended to each file

        bool IsEncrypted() const override { return (Flags & Flag_Encrypted) == Flag_Encrypted; }
        bool IsDeleted() const { return (Flags & Flag_Deleted) == Flag_Deleted; }
        Compression::CompressionMethod GetCompressionMethod() const override { return CompressionMethod; }
        bool IsCompressed() const { return UncompressedSize != CompressedSize && CompressionBlockSize > 0; }

        FPakEntry(VirtualFileSystem::IVfsReader* vfs, std::string path, int64_t size = 0)
            : VfsEntry(vfs, std::move(path), size) {}

        // Legacy index: the record is written out field by field.
        FPakEntry(PakFileReader& reader, const std::string& path, Readers::FArchive& Ar);

        // Updated index: the record is a bitfield at `offset` inside the encoded-entries blob.
        // UE4 reference: FPakFile::DecodePakEntry()
        FPakEntry(PakFileReader& reader, const std::string& path, Readers::FArchive& Ar, int offset);

        // Frozen index.
        FPakEntry(PakFileReader& reader, Readers::FMemoryImageArchive& Ar);

        // GameForPeace's custom record.
        FPakEntry(PakFileReader& reader, const std::string& path, Readers::FArchive& Ar, Versions::EGame game);

        PakFileReader& GetPakFileReader() const;

        std::vector<uint8_t> Read() override;
        std::unique_ptr<Readers::FArchive> CreateReader() override;
    };
}
