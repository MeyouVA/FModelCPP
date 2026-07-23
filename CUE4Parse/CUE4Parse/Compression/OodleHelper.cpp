// Ported (structurally) from CUE4Parse/Compression/OodleHelper.cs.
#include "OodleHelper.h"

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

        void* LoadLib(const char* name)
        {
#if defined(_WIN32)
            return reinterpret_cast<void*>(::LoadLibraryA(name));
#else
            return ::dlopen(name, RTLD_NOW);
#endif
        }

        void* GetSym(void* lib, const char* name)
        {
#if defined(_WIN32)
            return reinterpret_cast<void*>(::GetProcAddress(reinterpret_cast<HMODULE>(lib), name));
#else
            return ::dlsym(lib, name);
#endif
        }
    }

    bool OodleHelper::IsInitialized() { return g_decompress != nullptr; }

    bool OodleHelper::Initialize(const std::string& path)
    {
        if (g_decompress != nullptr) return true;

        void* lib = nullptr;
        if (!path.empty())
        {
            lib = LoadLib(path.c_str());
        }
        else
        {
#if defined(_WIN32)
            lib = LoadLib(OODLE_NAME_CURRENT);
            if (lib == nullptr) lib = LoadLib(OODLE_NAME_OLD);
#else
            lib = LoadLib(OODLE_NAME_LINUX);
#endif
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
