// Tests the Brotli path: BrotliHelper loading a native brotlidec at runtime and registering it with the
// Compression registry, then decoding reference-produced streams through Compression::Decompress.
//
// Brotli is the one codec C# gets for free (`System.IO.Compression.BrotliStream` is in the BCL), so there
// is no C# helper to mirror — only the algorithm's presence in the registry. Here it is a runtime library
// like Oodle and Zstd, and this suite reports SKIPPED when it is absent.
#include <cstdint>
#include <cstdio>
#include <vector>

#include "Compression/BrotliHelper.h"
#include "Compression/Compression.h"
#include "Compression/CompressionMethod.h"
#include "test_brotli_data.h"

using namespace CUE4Parse::Compression;

static int g_failures = 0;

#define CHECK(expr)                                                                     \
    do {                                                                                \
        if (!(expr)) {                                                                  \
            std::printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr);               \
            ++g_failures;                                                               \
        }                                                                               \
    } while (false)

static uint64_t Fnv1a64(const uint8_t* data, size_t size)
{
    uint64_t h = 0xcbf29ce484222325ull;
    for (size_t i = 0; i < size; ++i)
    {
        h ^= data[i];
        h *= 0x100000001b3ull;
    }
    return h;
}

int main()
{
    std::printf("=== test_brotli ===\n");

    // Before Initialize, nothing Brotli-shaped decodes — the registry has no entry for it.
    CHECK(!BrotliHelper::IsInitialized());
    CHECK(!Compression::HasDecompressor(CompressionAlgorithm::Brotli));

    if (!BrotliHelper::Initialize())
    {
        std::printf("SKIPPED: no brotlidec found (a runtime dependency, like Oodle and Zstd)\n");
        return g_failures == 0 ? 0 : 1;
    }
    std::printf("loaded brotlidec\n");
    CHECK(BrotliHelper::IsInitialized());
    CHECK(Compression::HasDecompressor(CompressionAlgorithm::Brotli));

    for (const auto& v : BrotliVectors::VECTORS)
    {
        const std::vector<uint8_t> compressed(v.Data, v.Data + v.Size);
        std::vector<uint8_t> out;
        try
        {
            out = Compression::Decompress(compressed, static_cast<int>(v.OriginalSize), CompressionMethod::Brotli);
        }
        catch (const std::exception& e)
        {
            std::printf("  FAIL %s: %s\n", v.Name, e.what());
            ++g_failures;
            continue;
        }
        if (out.size() != v.OriginalSize || Fnv1a64(out.data(), out.size()) != v.Hash)
        {
            std::printf("  FAIL %s: %zu bytes, hash mismatch\n", v.Name, out.size());
            ++g_failures;
            continue;
        }
        std::printf("  ok %-16s %zu -> %zu\n", v.Name, v.Size, v.OriginalSize);
    }

    // A truncated stream must be reported, not silently short-decoded.
    const auto& v = BrotliVectors::VECTORS[2];
    bool threw = false;
    try
    {
        const std::vector<uint8_t> truncated(v.Data, v.Data + v.Size - 50);
        Compression::Decompress(truncated, static_cast<int>(v.OriginalSize), CompressionMethod::Brotli);
    }
    catch (const std::exception&) { threw = true; }
    CHECK(threw);

    if (g_failures == 0) std::printf("ALL PASSED\n");
    else std::printf("%d FAILURE(S)\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
