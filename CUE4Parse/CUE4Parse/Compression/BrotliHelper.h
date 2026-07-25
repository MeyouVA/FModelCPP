// Structural analog of the Brotli path in CUE4Parse/Compression/Compression.cs.
//
// C# decompresses Brotli with `System.IO.Compression.BrotliStream`, straight out of the BCL, so there is no
// C# helper file to port — the algorithm is simply always available there. C++ has no such standard
// library, so Brotli follows the same shape as Oodle and Zstd here: a native `brotlidec` is loaded at
// runtime and registered with the Compression registry, and nothing Brotli-shaped decodes until it is.
//
// `brotlidec` depends on `brotlicommon`; keep both beside each other (ThirdParty/native/<platform> does),
// since LoadNativeLibrary loads by full path with the altered search path so the dependency resolves.
#pragma once

#include <string>

namespace CUE4Parse::Compression
{
    class BrotliHelper
    {
    public:
        // Default library names tried when Initialize is called with an empty path.
        static constexpr const char* BROTLI_NAME_WIN = "brotlidec.dll";
        static constexpr const char* BROTLI_NAME_LINUX = "libbrotlidec.so";

        // Load a native brotlidec (from `path`, or the default names if empty) and register the Brotli
        // decompressor. Returns true on success.
        static bool Initialize(const std::string& path = "");

        static bool IsInitialized();
    };
}
