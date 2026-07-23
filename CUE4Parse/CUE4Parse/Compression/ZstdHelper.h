// Structural analog of the Zstd path in CUE4Parse/Compression/Compression.cs.
//
// C# decompresses Zstd with the managed ZstdSharp package. The C++ port has no such dependency and a full
// Zstd decoder is too large to vendor here, so — like OodleHelper — this loads a native libzstd at runtime
// and registers ZSTD_decompress with the Compression registry.
#pragma once

#include <string>

namespace CUE4Parse::Compression
{
    class ZstdHelper
    {
    public:
        static constexpr const char* ZSTD_NAME_WIN = "libzstd.dll";
        static constexpr const char* ZSTD_NAME_WIN_ALT = "zstd.dll";
        static constexpr const char* ZSTD_NAME_LINUX = "libzstd.so";

        // Load a native zstd library (from `path`, or the default names if empty) and register the Zstd
        // decompressor. Returns true on success.
        static bool Initialize(const std::string& path = "");

        static bool IsInitialized();
    };
}
