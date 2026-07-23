// Ported from CUE4Parse/UE4/VirtualFileCache/Manifest/EManifestStorageFlags.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::VirtualFileCache::Manifest
{
    enum class EManifestStorageFlags : uint8_t
    {
        // Stored as raw data.
        None       = 0,
        // Flag for compressed data.
        Compressed = 1,
        // Flag for encrypted. If also compressed, decrypt first. Encryption will ruin compressibility.
        Encrypted  = 1u << 1,
    };
}
