// Ported (structurally) from CUE4Parse/Compression/OodleHelper.cs.
#include "OodleHelper.h"
#include "NativeLibrary.h"

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
        // OodleLZ_Decompress(const void* src, intptr_t srcLen, void* dst, intptr_t dstLen,
        //   int fuzzSafe, int checkCRC, int verbosity, void* decBufBase, intptr_t decBufSize,
        //   void* fpCallback, void* callbackUserData, void* decoderMemory, intptr_t decoderMemorySize, int threadPhase)
        using OodleLZ_Decompress_t = int64_t (*)(const void*, int64_t, void*, int64_t,
                                                 int, int, int, void*, int64_t,
                                                 void*, void*, void*, int64_t, int);

        OodleLZ_Decompress_t g_decompress = nullptr;

        // Both live in NativeLibrary.cpp now — see the note there about dependent DLLs.
        void* LoadLib(const char* name) { return LoadNativeLibrary(name); }
        void* GetSym(void* lib, const char* name) { return GetNativeSymbol(lib, name); }
    }

    bool OodleHelper::IsInitialized() { return g_decompress != nullptr; }

    bool OodleHelper::Initialize(const std::string& path)
    {
        if (g_decompress != nullptr) return true;

        // Every place worth looking, in NativeLibrary.h's documented order — the caller's path first, then
        // the env override, the executable's own directory and the in-tree ThirdParty/native folder, and
        // finally the bare name for the OS loader. No absolute path is compiled in.
        void* lib = nullptr;
#if defined(_WIN32)
        const char* const names[] = {OODLE_NAME_CURRENT, OODLE_NAME_OLD};
#else
        const char* const names[] = {OODLE_NAME_LINUX};
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

        auto fn = reinterpret_cast<OodleLZ_Decompress_t>(GetSym(lib, "OodleLZ_Decompress"));
        if (fn == nullptr) return false;
        g_decompress = fn;

        Compression::RegisterDecompressor(CompressionAlgorithm::Oodle,
            [](const uint8_t* src, int srcLen, uint8_t* dst, int dstLen, int& written)
            {
                // fuzzSafe = OodleLZ_FuzzSafe_Yes(1), checkCRC = No(0), verbosity = None(0),
                // threadPhase = OodleLZ_Decode_Unthreaded(3).
                const int64_t r = g_decompress(src, srcLen, dst, dstLen, 1, 0, 0,
                                               nullptr, 0, nullptr, nullptr, nullptr, 0, 3);
                written = static_cast<int>(r);
                return r > 0;
            });
        return true;
    }
}
