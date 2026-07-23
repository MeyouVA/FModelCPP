#include "FIoStoreTocHeader.h"

#include "../../Exceptions/ParserException.h"
#include "../../../Utils/AlignUtils.h"

namespace CUE4Parse::UE4::IO::Objects
{
    const std::array<uint8_t, 16> FIoStoreTocHeader::TOC_MAGIC = {
        0x2D, 0x3D, 0x3D, 0x2D, 0x2D, 0x3D, 0x3D, 0x2D,
        0x2D, 0x3D, 0x3D, 0x2D, 0x2D, 0x3D, 0x3D, 0x2D}; // -==--==--==--==-

    FIoStoreTocHeader::FIoStoreTocHeader(Readers::FArchive& Ar)
    {
        Ar.Serialize(TocMagic.data(), static_cast<int>(TocMagic.size()));
        if (TocMagic != TOC_MAGIC)
            throw Exceptions::ParserException(Ar, "Invalid utoc magic");
        Version = Ar.Read<EIoStoreTocVersion>();
        Ar.Read<uint8_t>();  // _reserved0
        Ar.Read<uint16_t>(); // _reserved1
        TocHeaderSize = Ar.Read<uint32_t>();
        TocEntryCount = Ar.Read<uint32_t>();
        TocCompressedBlockEntryCount = Ar.Read<uint32_t>();
        TocCompressedBlockEntrySize = Ar.Read<uint32_t>();
        CompressionMethodNameCount = Ar.Read<uint32_t>();
        CompressionMethodNameLength = Ar.Read<uint32_t>();
        CompressionBlockSize = Ar.Read<uint32_t>();
        DirectoryIndexSize = Ar.Read<uint32_t>();
        PartitionCount = Ar.Read<uint32_t>();
        ContainerId = Ar.Read<FIoContainerId>();
        EncryptionKeyGuid = Ar.Read<UE4::Objects::Core::Misc::FGuid>();
        ContainerFlags = Ar.Read<EIoContainerFlags>();
        TocChunkPerfectHashSeedsCount = Ar.Read<uint32_t>();
        PartitionSize = Ar.Read<uint64_t>();
        TocChunksWithoutPerfectHashCount = Ar.Read<uint32_t>();
        Ar.Read<uint32_t>(); // _reserved7
        for (int i = 0; i < 5; ++i) Ar.Read<uint64_t>(); // _reserved8
        Ar.Position = Utils::Align(Ar.Position, 4);
    }
}
