// Ported from CUE4Parse/UE4/IO/Objects/FIoStoreTocEntryMeta.cs (+ FIoChunkHash.cs)
// Per-chunk hash + flags, only read under EIoStoreTocReadOptions::ReadTocMeta.
//
// C# stores the hash as FSHAHash (20 bytes) or the older FIoChunkHash (32 bytes) behind one field; here
// one 32-byte buffer holds either, with the SHA1 form occupying the first 20 bytes.
#pragma once

#include <array>
#include <cstdint>

#include "../../Readers/FArchive.h"

namespace CUE4Parse::UE4::IO::Objects
{
    enum class FIoStoreTocEntryMetaFlags : uint8_t
    {
        None,
        Compressed = 1 << 0,
        MemoryMapped = 1 << 1,
    };

    struct FIoStoreTocEntryMeta
    {
        std::array<uint8_t, 32> ChunkHash{};
        FIoStoreTocEntryMetaFlags Flags = FIoStoreTocEntryMetaFlags::None;

        FIoStoreTocEntryMeta() = default;
        FIoStoreTocEntryMeta(Readers::FArchive& Ar, bool replacedIoChunkHashWithIoHash)
        {
            const int hashSize = replacedIoChunkHashWithIoHash ? 20 : 32; // FSHAHash vs FIoChunkHash
            Ar.Serialize(ChunkHash.data(), hashSize);
            Flags = Ar.Read<FIoStoreTocEntryMetaFlags>();
            if (replacedIoChunkHashWithIoHash) Ar.Position += 3;
        }
    };
}
