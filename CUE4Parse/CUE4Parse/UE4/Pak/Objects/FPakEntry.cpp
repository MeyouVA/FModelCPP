#include "FPakEntry.h"

#include <utility>

#include "../PakFileReader.h"
#include "../../Exceptions/ParserException.h"
#include "../../Objects/Core/Misc/ECompressionFlags.h"
#include "../../Objects/Core/Misc/FSHAHash.h"
#include "../../Readers/FByteArchive.h"
#include "../../../Encryption/Aes/Aes.h"
#include "../../../Utils/AlignUtils.h"
#include "../../../Utils/StringUtils.h"

namespace CUE4Parse::UE4::Pak::Objects
{
    using namespace CUE4Parse::UE4::Versions;
    using namespace CUE4Parse::UE4::Objects::Core::Misc;
    using CUE4Parse::Compression::CompressionMethod;
    using CUE4Parse::Encryption::Aes::Aes;

    namespace
    {
        bool EndsWith(const std::string& s, const std::string& suffix)
        {
            return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
        }
    }

    FPakEntry::FPakEntry(PakFileReader& reader, const std::string& path, Readers::FArchive& Ar)
        : VfsEntry(&reader, path)
    {
        // FPakEntry is duplicated before each stored file, without a filename. So,
        // remember the serialized size of this structure to avoid recomputation later.
        const int64_t startOffset = Ar.Position;

        Offset = Ar.Read<int64_t>();

        if (Ar.Game() == GAME_GearsOfWar4)
        {
            CompressedSize = Ar.Read<int32_t>();
            UncompressedSize = Ar.Read<int32_t>();
            CompressionMethod = static_cast<Compression::CompressionMethod>(Ar.Read<uint8_t>());

            if (reader.Info.Version < EPakFileVersion::PakFile_Version_NoTimestamps)
                Ar.Position += 8;

            if (reader.Info.Version >= EPakFileVersion::PakFile_Version_CompressionEncryption)
            {
                if (CompressionMethod != CompressionMethod::None)
                    CompressionBlocks = Ar.ReadArrayCounted<FPakCompressedBlock>();
                CompressionBlockSize = Ar.Read<uint32_t>();
                if (CompressionMethod == CompressionMethod::Oodle)
                    CompressionMethod = CompressionMethod::LZ4;
            }

            goto endRead;
        }

        CompressedSize = Ar.Read<int64_t>();
        UncompressedSize = Ar.Read<int64_t>();
        Size = UncompressedSize;

        if (reader.Info.Version < EPakFileVersion::PakFile_Version_FNameBasedCompressionMethod)
        {
            const ECompressionFlags legacyCompressionMethod = Ar.Read<ECompressionFlags>();
            int compressionMethodIndex;
            if (legacyCompressionMethod == COMPRESS_None) compressionMethodIndex = 0;
            else if (legacyCompressionMethod == static_cast<ECompressionFlags>(259)) compressionMethodIndex = 4; // SOD2
            else if ((legacyCompressionMethod & COMPRESS_ZLIB) != 0) compressionMethodIndex = 1;
            else if ((legacyCompressionMethod & COMPRESS_GZIP) != 0) compressionMethodIndex = 2;
            else if ((legacyCompressionMethod & COMPRESS_Custom) != 0)
                compressionMethodIndex = reader.Game() == GAME_SeaOfThieves ? 4 : 3; // LZ4 or Oodle, used by Fortnite Mobile until early 2019
            else
            {
                switch (reader.Game())
                {
                    case GAME_PlayerUnknownsBattlegrounds:
                    case GAME_Ashen: compressionMethodIndex = 3; break; // TODO: Investigate what a proper detection is.
                    case GAME_DeadIsland2: compressionMethodIndex = 6; break; // ¯\_(ツ)_/¯
                    default: compressionMethodIndex = -1; break;
                }
            }
            CompressionMethod = compressionMethodIndex == -1
                ? CompressionMethod::Unknown
                : reader.Info.CompressionMethods[static_cast<size_t>(compressionMethodIndex)];
        }
        else if (reader.Info.Version == EPakFileVersion::PakFile_Version_FNameBasedCompressionMethod && !reader.Info.IsSubVersion)
        {
            CompressionMethod = reader.Info.CompressionMethods[Ar.Read<uint8_t>()];
        }
        else
        {
            CompressionMethod = reader.Info.CompressionMethods[static_cast<size_t>(Ar.Read<int32_t>())];
        }

        if (reader.Info.Version < EPakFileVersion::PakFile_Version_NoTimestamps)
            Ar.Position += 8; // Timestamp
        Ar.Position += 20; // Hash

        if (reader.Info.Version >= EPakFileVersion::PakFile_Version_CompressionEncryption)
        {
            if (CompressionMethod != CompressionMethod::None)
                CompressionBlocks = Ar.ReadArrayCounted<FPakCompressedBlock>();

            if (Ar.Game() == GAME_Back4Blood)
            {
                CompressionBlockSize = Ar.Read<uint32_t>();
                Flags = Ar.Read<uint8_t>();
            }
            else
            {
                Flags = Ar.Read<uint8_t>();
                CompressionBlockSize = Ar.Read<uint32_t>();
            }

            if (Ar.Game() == GAME_ConanExiles)
            {
                if (CompressionMethod != CompressionMethod::None && (EndsWith(path, "gtp") || EndsWith(path, "gts")))
                {
                    // Conan Exiles has a bug where it stores the CompressionBlocks for gpt files, but doesn't use them.
                    // It also doesn't use CompressionBlockSize, so we can ignore it.
                    CompressionBlocks.clear();
                    CompressionBlockSize = 0;
                    CompressionMethod = CompressionMethod::None;
                }
                Ar.Position += 4;
            }
        }

        if (Ar.Game() == GAME_TEKKEN7) Flags = Flags & ~static_cast<uint32_t>(Flag_Encrypted);

        if (reader.Info.Version >= EPakFileVersion::PakFile_Version_RelativeChunkOffsets)
        {
            // Convert relative compressed offsets to absolute
            for (auto& block : CompressionBlocks)
            {
                block.CompressedStart += Offset;
                block.CompressedEnd += Offset;
            }
        }

        endRead:
        StructSize = static_cast<int32_t>(Ar.Position - startOffset);

        if (Ar.Game() == GAME_StateOfDecay2 && CompressionMethod == CompressionMethod::None) StructSize = 0;
    }

    FPakEntry::FPakEntry(PakFileReader& reader, const std::string& path, Readers::FArchive& Ar, int offset)
        : VfsEntry(&reader, path)
    {
        // UE4 reference: FPakFile::DecodePakEntry()
        Ar.Seek(offset, Readers::ESeekOrigin::Begin);
        uint32_t bitfield = Ar.Read<uint32_t>();

        if (reader.Game() == GAME_WutheringWaves && reader.Info.Version > EPakFileVersion::PakFile_Version_Fnv64BugFix)
        {
            bitfield = (bitfield >> 16) & 0x3F | (bitfield & 0xFFFF) << 6 | (bitfield & (1u << 28)) >> 6 |
                       (bitfield & 0x0FC00000u) << 1 | (bitfield & 0xC0000000u) >> 1 | (bitfield & 0x20000000u) << 2;
            CustomData = Ar.Read<uint8_t>();
        }

        if (reader.Game() == GAME_InfinityNikki)
        {
            const uint32_t compressionBlocksNum = (bitfield >> 6) & 0xFFFF;
            const uint32_t isOffset32BitSafe = (bitfield >> 31) & 1;
            const uint32_t isSize32BitSafe = (bitfield >> 22) & 1;
            const uint32_t isUncompressedSize32BitSafe = (bitfield >> 30) & 1;
            const uint32_t compressedSizeBacked = bitfield & 0x3F;
            const uint32_t isEncrypted = (bitfield >> 29) & 1;
            const uint32_t compressionMethodIndex = (bitfield >> 23) & 0x3F;

            bitfield = compressedSizeBacked
                       | (compressionBlocksNum << 6)
                       | (isEncrypted << 22)
                       | (compressionMethodIndex << 23)
                       | (isSize32BitSafe << 29)
                       | (isUncompressedSize32BitSafe << 30)
                       | (isOffset32BitSafe << 31);
        }

        // ValorantSource skips an FSHAHash here and reconstructs offset/size through the Tencent nibble
        // masks; that encryption is not ported (see FPakInfo).
        if (reader.Game() == GAME_ValorantSource)
            throw Exceptions::ParserException("ValorantSource pak entries need the unported Tencent encryption");

        const uint32_t compressionBlockSize = (bitfield & 0x3f) == 0x3f ? Ar.Read<uint32_t>() : (bitfield & 0x3f) << 11;

        // Filter out the CompressionMethod.
        CompressionMethod = reader.Info.CompressionMethods[static_cast<size_t>((bitfield >> 23) & 0x3f)];

        // Read the Offset.
        const bool bIsOffset32BitSafe = (bitfield & (1u << 31)) != 0;
        const bool bIsUncompressedSize32BitSafe = (bitfield & (1u << 30)) != 0;
        Offset = bIsOffset32BitSafe ? static_cast<int64_t>(Ar.Read<uint32_t>()) : Ar.Read<int64_t>(); // Should be ulong
        UncompressedSize = bIsUncompressedSize32BitSafe ? static_cast<int64_t>(Ar.Read<uint32_t>()) : Ar.Read<int64_t>(); // Should be ulong

        if (reader.Game() == GAME_Snowbreak) Offset ^= 0x1F1E1D1C;
        if (reader.Game() == GAME_QQ || reader.Game() == GAME_DreamStar) Offset += 8;

        if (reader.Game() == GAME_WutheringWaves && reader.Info.Version > EPakFileVersion::PakFile_Version_Fnv64BugFix)
            std::swap(Offset, UncompressedSize);

        Size = UncompressedSize;

        // Fill in the Size.
        if (CompressionMethod != CompressionMethod::None)
        {
            const bool bIsSize32BitSafe = (bitfield & (1u << 29)) != 0;
            CompressedSize = bIsSize32BitSafe ? static_cast<int64_t>(Ar.Read<uint32_t>()) : Ar.Read<int64_t>();
        }
        else
        {
            // The Size is the same thing as the UncompressedSize when
            // CompressionMethod == CompressionMethod.None.
            CompressedSize = UncompressedSize;
        }

        // Filter the encrypted flag.
        Flags |= (bitfield & (1u << 22)) != 0 ? 1u : 0u;

        // This should clear out any excess CompressionBlocks that may be valid in the user's passed in entry.
        const uint32_t compressionBlocksCount = (bitfield >> 6) & 0xffff;
        CompressionBlocks.assign(static_cast<size_t>(compressionBlocksCount), FPakCompressedBlock());
        CompressionBlockSize = compressionBlocksCount == 1
            ? static_cast<uint32_t>(UncompressedSize)
            : (compressionBlocksCount > 0 ? compressionBlockSize : 0u);

        // Compute StructSize: each file still have FPakEntry data prepended, and it should be skipped.
        StructSize = sizeof(int64_t) * 3 + sizeof(int32_t) * 2 + 1 + 20;
        // Take into account CompressionBlocks
        if (CompressionMethod != CompressionMethod::None)
            StructSize += static_cast<int32_t>(sizeof(int32_t) + compressionBlocksCount * 2 * sizeof(int64_t));

        switch (reader.Ar->Game())
        {
            case GAME_TorchlightInfinite:
            case GAME_EtheriaRestart: StructSize += 1; break;
            case GAME_BlackMythWukong: StructSize += 1; break;
            case GAME_InfinityNikki: StructSize += 20; break;
            case GAME_VisionsofMana: StructSize += -3; break;
            default: break;
        }

        // Handle building of the CompressionBlocks array.
        int64_t compressedBlockOffset = Offset + StructSize;
        if (compressionBlocksCount == 1 && !IsEncrypted())
        {
            FPakCompressedBlock& b = CompressionBlocks[0];
            b.CompressedStart = compressedBlockOffset;
            b.CompressedEnd = compressedBlockOffset + CompressedSize;
        }
        else if (compressionBlocksCount > 0)
        {
            const int compressedBlockAlignment = IsEncrypted() ? Aes::ALIGN : 1;
            for (uint32_t compressionBlockIndex = 0; compressionBlockIndex < compressionBlocksCount; ++compressionBlockIndex)
            {
                FPakCompressedBlock& compressedBlock = CompressionBlocks[compressionBlockIndex];
                const uint32_t length = Ar.Read<uint32_t>();
                compressedBlock.CompressedStart = compressedBlockOffset;
                compressedBlock.CompressedEnd = compressedBlockOffset + length;
                compressedBlockOffset += Utils::Align(length, compressedBlockAlignment);
            }
        }
    }

    FPakEntry::FPakEntry(PakFileReader& reader, Readers::FMemoryImageArchive& Ar)
        : VfsEntry(&reader)
    {
        Offset = Ar.Read<int64_t>();
        CompressedSize = Ar.Read<int64_t>();
        UncompressedSize = Ar.Read<int64_t>();
        Size = UncompressedSize;
        Ar.Position += FSHAHash::SIZE + 4 /*align to 8 bytes*/; //Hash = new FSHAHash(Ar);
        CompressionBlocks = Ar.ReadArrayCounted<FPakCompressedBlock>();
        CompressionBlockSize = Ar.Read<uint32_t>();
        CompressionMethod = reader.Info.CompressionMethods[static_cast<size_t>(Ar.Read<int32_t>())];
        Flags = Ar.Read<uint8_t>();

        if (reader.Info.Version >= EPakFileVersion::PakFile_Version_RelativeChunkOffsets)
        {
            // Convert relative compressed offsets to absolute
            for (auto& block : CompressionBlocks)
            {
                block.CompressedStart += Offset;
                block.CompressedEnd += Offset;
            }
        }

        // Compute StructSize: each file still have FPakEntry data prepended, and it should be skipped.
        StructSize = sizeof(int64_t) * 3 + sizeof(int32_t) * 2 + 1 + 20;
        // Take into account CompressionBlocks
        if (CompressionMethod != CompressionMethod::None)
            StructSize += static_cast<int32_t>(sizeof(int32_t) + CompressionBlocks.size() * 2 * sizeof(int64_t));
    }

    FPakEntry::FPakEntry(PakFileReader& reader, const std::string& path, Readers::FArchive& Ar, EGame game)
        : VfsEntry(&reader, path)
    {
        const int64_t startOffset = Ar.Position;

        if (game == GAME_GameForPeace)
        {
            Ar.Position += 20;
            Offset = Ar.Read<int64_t>();
            UncompressedSize = Ar.Read<int64_t>();
            CompressionMethod = reader.Info.CompressionMethods[static_cast<size_t>(Ar.Read<int32_t>())];
            CompressedSize = Ar.Read<int64_t>();
            Size = UncompressedSize;
            Ar.Position += 21;
            if (CompressionMethod != CompressionMethod::None)
                CompressionBlocks = Ar.ReadArrayCounted<FPakCompressedBlock>();
            CompressionBlockSize = Ar.Read<uint32_t>();
            Flags = Ar.Read<uint8_t>();
        }

        StructSize = static_cast<int32_t>(Ar.Position - startOffset);
    }

    PakFileReader& FPakEntry::GetPakFileReader() const
    {
        // dynamic_cast, not static_cast: IVfsReader is a virtual base, so the downcast needs RTTI.
        return *dynamic_cast<PakFileReader*>(Vfs);
    }

    std::vector<uint8_t> FPakEntry::Read()
    {
        return Vfs->Extract(*this);
    }

    std::unique_ptr<Readers::FArchive> FPakEntry::CreateReader()
    {
        return std::make_unique<Readers::FByteArchive>(Path(), Read(), Vfs->GetVersions());
    }
}
