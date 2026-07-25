// See BrotliHelper.h.
#include "BrotliHelper.h"
#include "NativeLibrary.h"

#include <cstddef>
#include <cstdint>

#include "Compression.h"

namespace CUE4Parse::Compression
{
    namespace
    {
        // BrotliDecoderResult BrotliDecoderDecompress(size_t encodedSize, const uint8_t* encodedBuffer,
        //                                             size_t* decodedSize, uint8_t* decodedBuffer);
        // The one-shot entry point: BROTLI_DECODER_RESULT_SUCCESS is 1, and `decodedSize` is in/out —
        // capacity going in, bytes produced coming out.
        using BrotliDecoderDecompress_t = int (*)(size_t, const uint8_t*, size_t*, uint8_t*);

        BrotliDecoderDecompress_t g_decompress = nullptr;

        constexpr int BROTLI_DECODER_RESULT_SUCCESS = 1;
    }

    bool BrotliHelper::IsInitialized() { return g_decompress != nullptr; }

    bool BrotliHelper::Initialize(const std::string& path)
    {
        if (g_decompress != nullptr) return true;

        // Same search order as Oodle and Zstd — see NativeLibrary.h. Nothing machine-specific is compiled in.
        void* lib = nullptr;
#if defined(_WIN32)
        const char* const names[] = {BROTLI_NAME_WIN};
#else
        const char* const names[] = {BROTLI_NAME_LINUX};
#endif
        for (const char* name : names)
        {
            for (const std::string& candidate : NativeLibraryCandidates(name, path))
            {
                lib = LoadNativeLibrary(candidate);
                if (lib != nullptr) break;
            }
            if (lib != nullptr) break;
        }
        if (lib == nullptr) return false;

        auto fn = reinterpret_cast<BrotliDecoderDecompress_t>(GetNativeSymbol(lib, "BrotliDecoderDecompress"));
        if (fn == nullptr) return false;
        g_decompress = fn;

        Compression::RegisterDecompressor(CompressionAlgorithm::Brotli,
            [](const uint8_t* src, int srcLen, uint8_t* dst, int dstLen, int& written)
            {
                size_t produced = static_cast<size_t>(dstLen);
                const int r = g_decompress(static_cast<size_t>(srcLen), src, &produced, dst);
                if (r != BROTLI_DECODER_RESULT_SUCCESS) { written = 0; return false; }
                written = static_cast<int>(produced);
                return true;
            });
        return true;
    }
}
