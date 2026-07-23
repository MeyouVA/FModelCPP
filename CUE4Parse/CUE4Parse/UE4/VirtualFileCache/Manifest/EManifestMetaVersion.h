// Ported from CUE4Parse/UE4/VirtualFileCache/Manifest/EManifestMetaVersion.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::VirtualFileCache::Manifest
{
    enum class EManifestMetaVersion : uint8_t
    {
        Original          = 0,
        SerialisesBuildId,
        // Always after the latest version, signifies the latest version plus 1 to allow initialization simplicity.
        LatestPlusOne,
        Latest            = LatestPlusOne - 1,
    };
}
