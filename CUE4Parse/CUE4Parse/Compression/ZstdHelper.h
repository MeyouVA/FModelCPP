// Structural analog of the Zstd path in CUE4Parse/Compression/Compression.cs.
//
// C# decompresses Zstd with the managed ZstdSharp package (a from-scratch managed port of the reference
// decoder). The C++ port uses the reference implementation itself instead: this loads a native libzstd at
// runtime — exactly as OodleHelper does for Oodle — and registers ZSTD_decompress with the Compression
// registry. Nothing Zstd-shaped is decoded until Initialize succeeds.
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
