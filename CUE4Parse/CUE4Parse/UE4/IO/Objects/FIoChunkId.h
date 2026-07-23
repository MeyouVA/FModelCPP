// Ported from CUE4Parse/UE4/IO/Objects/FIoChunkId.cs
// The 12-byte chunk address: 8-byte id, big-endian chunk index, pad, chunk type. Read straight off the toc
// with ReadArray, so the layout is pinned to exactly 12 bytes (C#'s Pack = 1).
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

namespace CUE4Parse::UE4::VirtualFileSystem { class IAesVfsReader; }

namespace CUE4Parse::UE4::IO::Objects
{
    // Addressable chunk types (UE4).
    enum class EIoChunkType : uint8_t
    {
        Invalid,
        InstallManifest,
        ExportBundleData,
        BulkData,
        OptionalBulkData,
        MemoryMappedBulkData,
        LoaderGlobalMeta,
        LoaderInitialLoadMeta,
        LoaderGlobalNames,
        LoaderGlobalNameHashes,
        ContainerHeader,
    };

    // Addressable chunk types (UE5); values are explicit for forward compatibility.
    enum class EIoChunkType5 : uint8_t
    {
        Invalid = 0,
        ExportBundleData = 1,
        BulkData = 2,
        OptionalBulkData = 3,
        MemoryMappedBulkData = 4,
        ScriptObjects = 5,
        ContainerHeader = 6,
        ExternalFile = 7,
        ShaderCodeLibrary = 8,
        ShaderCode = 9,
        PackageStoreEntry = 10,
        DerivedData = 11,
        EditorDerivedData = 12,
        PackageResource = 13,
        MAX,
    };

    struct FPackageId;

#pragma pack(push, 1)
    struct FIoChunkId
    {
        uint64_t ChunkId = 0;
        uint16_t _chunkIndex = 0; // stored big-endian (NETWORK_ORDER16)
        uint8_t _padding = 0;
        uint8_t ChunkType = 0;

        FIoChunkId() = default;
        FIoChunkId(uint64_t chunkId, uint16_t chunkIndex, uint8_t chunkType)
            : ChunkId(chunkId),
              _chunkIndex(static_cast<uint16_t>((chunkIndex & 0xFF) << 8 | (chunkIndex & 0xFF00) >> 8)),
              ChunkType(chunkType) {}
        FIoChunkId(uint64_t chunkId, uint16_t chunkIndex, EIoChunkType chunkType)
            : FIoChunkId(chunkId, chunkIndex, static_cast<uint8_t>(chunkType)) {}
        FIoChunkId(uint64_t chunkId, uint16_t chunkIndex, EIoChunkType5 chunkType)
            : FIoChunkId(chunkId, chunkIndex, static_cast<uint8_t>(chunkType)) {}

        FPackageId AsPackageId() const; // defined in FIoChunkId.cpp (FPackageId include cycle)

        // File extension for a chunk of this type under the reader's engine generation. Defined in
        // FIoChunkId.cpp (needs IoStoreReader for the uasset-vs-uexp distinction).
        std::string GetExtension(const VirtualFileSystem::IAesVfsReader& reader) const;

        // FNV-1a over the 12 raw bytes; the perfect-hash lookup keys on this.
        uint64_t HashWithSeed(int32_t seed) const
        {
            const auto* bytes = reinterpret_cast<const uint8_t*>(this);
            uint64_t hash = seed != 0
                ? static_cast<uint64_t>(static_cast<int64_t>(seed)) // C# sign-extends the negative-seed cast
                : 0xcbf29ce484222325ULL;
            for (size_t i = 0; i < sizeof(FIoChunkId); ++i)
                hash = (hash * 0x00000100000001B3ULL) ^ bytes[i];
            return hash;
        }

        // C#'s GetHashCode (djb2 over the 12 bytes). TryResolve compares these instead of full equality,
        // which is kept verbatim.
        int32_t HashCode() const
        {
            const auto* bytes = reinterpret_cast<const uint8_t*>(this);
            int32_t hash = 5381;
            for (size_t i = 0; i < sizeof(FIoChunkId); ++i)
                hash = static_cast<int32_t>(static_cast<uint32_t>(hash) * 33u + bytes[i]);
            return hash;
        }

        // C#'s Equals compares id + type only (not the index), kept verbatim.
        bool operator==(const FIoChunkId& other) const
        {
            return ChunkId == other.ChunkId && ChunkType == other.ChunkType;
        }

        std::string ToString() const
        {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "0x%08llX | %u",
                          static_cast<unsigned long long>(ChunkId), static_cast<unsigned>(ChunkType));
            return buf;
        }
    };
#pragma pack(pop)
    static_assert(sizeof(FIoChunkId) == 12, "FIoChunkId must match the 12-byte on-disk layout");
}
