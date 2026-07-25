// Structural analog of the Zstd path in CUE4Parse/Compression/Compression.cs.
#include "ZstdHelper.h"
#include "NativeLibrary.h"

#include <cstddef>
#include <cstdint>

#include "Compression.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace CUE4Parse::Compression
{
    namespace
    {
        // size_t ZSTD_decompress(void* dst, size_t dstCapacity, const void* src, size_t srcSize);
        using ZSTD_decompress_t = size_t (*)(void*, size_t, const void*, size_t);
        // unsigned ZSTD_isError(size_t code);
        using ZSTD_isError_t = unsigned (*)(size_t);

        ZSTD_decompress_t g_decompress = nullptr;
        ZSTD_isError_t g_isError = nullptr;

        // Both live in NativeLibrary.cpp now — see the note there about dependent DLLs.
        void* LoadLib(const char* name) { return LoadNativeLibrary(name); }
        void* GetSym(void* lib, const char* name) { return GetNativeSymbol(lib, name); }
    }

    bool ZstdHelper::IsInitialized() { return g_decompress != nullptr; }

    bool ZstdHelper::Initialize(const std::string& path)
    {
        if (g_decompress != nullptr) return true;

        // Same search order as Oodle — see NativeLibrary.h. Nothing machine-specific is compiled in.
        void* lib = nullptr;
#if defined(_WIN32)
        const char* const names[] = {ZSTD_NAME_WIN, ZSTD_NAME_WIN_ALT};
#else
        const char* const names[] = {ZSTD_NAME_LINUX};
#endif
        for (const char* name : names)
        {
            for (const std::string& candidate : NativeLibraryCandidates(name, path))
            {
                lib = LoadLib(candidate.c_str());
                if (lib != nullptr) break;
            }
            if (lib != nullptr) break;
        }
        if (lib == nullptr) return false;

        auto dec = reinterpret_cast<ZSTD_decompress_t>(GetSym(lib, "ZSTD_decompress"));
        auto err = reinterpret_cast<ZSTD_isError_t>(GetSym(lib, "ZSTD_isError"));
        if (dec == nullptr || err == nullptr) return false;
        g_decompress = dec;
        g_isError = err;

        // Mirrors the C# Zstd delegate: ZSTD_decompress then ZSTD_isError check.
        Compression::RegisterDecompressor(CompressionAlgorithm::Zstd,
            [](const uint8_t* src, int srcLen, uint8_t* dst, int dstLen, int& written)
            {
                const size_t r = g_decompress(dst, static_cast<size_t>(dstLen), src, static_cast<size_t>(srcLen));
                if (g_isError(r)) { written = 0; return false; }
                written = static_cast<int>(r);
                return true;
            });
        return true;
    }
}
