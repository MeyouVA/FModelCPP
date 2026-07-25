// Tests the Zstd path: ZstdHelper loading a native libzstd at runtime and registering it with the
// Compression registry, then decoding real frames through Compression::Decompress.
//
// The vectors in test_zstd_data.h were produced by the reference implementation and cover raw blocks
// (incompressible input), RLE blocks and repeat offsets (highly repetitive input), Huffman literals in
// both the 1-stream and 4-stream layouts, FSE-compressed Huffman weights, entropy tables reused across
// blocks (level 19), a content checksum, a skippable frame, and two frames back to back.
//
// If no libzstd can be found the suite reports SKIPPED and passes: the library is a runtime dependency,
// exactly like Oodle, and a machine without it is not a broken build.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "Compression/Compression.h"
#include "Compression/CompressionMethod.h"
#include "Compression/ZstdHelper.h"
#include "test_zstd_data.h"

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

// ZstdHelper::Initialize("") already probes, in order: $FMODELCPP_NATIVE_DIR, the executable's own
// directory (where CMake copies ThirdParty/native/<platform>), the in-tree ThirdParty folder walking up
// from the build tree, and finally the bare library name for the OS loader. Nothing machine-specific is
// spelled out here — drop a libzstd in ThirdParty/native/win-x64 and this finds it.
static bool LoadZstd()
{
    if (!ZstdHelper::Initialize()) return false;
    std::printf("loaded libzstd\n");
    return true;
}

static void TestVectors()
{
    std::printf("TestVectors\n");
    for (const auto& v : ZstdVectors::VECTORS)
    {
        const std::vector<uint8_t> compressed(v.Data, v.Data + v.Size);
        std::vector<uint8_t> out;
        try
        {
            out = Compression::Decompress(compressed, static_cast<int>(v.OriginalSize), CompressionMethod::Zstd);
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
        std::printf("  ok %-18s %zu -> %zu\n", v.Name, v.Size, v.OriginalSize);
    }

    // The smallest vector, byte for byte.
    const auto& tiny = ZstdVectors::VECTORS[5];
    const std::vector<uint8_t> compressed(tiny.Data, tiny.Data + tiny.Size);
    const std::vector<uint8_t> out = Compression::Decompress(compressed, static_cast<int>(tiny.OriginalSize),
                                                             CompressionMethod::Zstd);
    CHECK(out.size() == sizeof(ZstdVectors::TINY_ORIGINAL));
    CHECK(std::memcmp(out.data(), ZstdVectors::TINY_ORIGINAL, sizeof(ZstdVectors::TINY_ORIGINAL)) == 0);
}

static void TestFailureModes()
{
    std::printf("TestFailureModes\n");
    const auto& v = ZstdVectors::VECTORS[2];   // text_level3
    const std::vector<uint8_t> full(v.Data, v.Data + v.Size);

    // Truncated input, and a destination too small: both must throw rather than hand back a short buffer.
    bool threw = false;
    try { const std::vector<uint8_t> t(v.Data, v.Data + v.Size - 100);
          Compression::Decompress(t, static_cast<int>(v.OriginalSize), CompressionMethod::Zstd); }
    catch (const std::exception&) { threw = true; }
    CHECK(threw);

    threw = false;
    try { Compression::Decompress(full, 64, CompressionMethod::Zstd); }
    catch (const std::exception&) { threw = true; }
    CHECK(threw);

    // Not a Zstd frame at all.
    threw = false;
    try { const std::vector<uint8_t> garbage(32, 0xAB);
          Compression::Decompress(garbage, 100, CompressionMethod::Zstd); }
    catch (const std::exception&) { threw = true; }
    CHECK(threw);
}

int main()
{
    std::printf("=== test_zstd ===\n");

    // Before Initialize, nothing Zstd-shaped decodes — the registry has no entry for it.
    CHECK(!ZstdHelper::IsInitialized());
    CHECK(!Compression::HasDecompressor(CompressionAlgorithm::Zstd));

    if (!LoadZstd())
    {
        std::printf("SKIPPED: no libzstd found (this is a runtime dependency, like Oodle)\n");
        return g_failures == 0 ? 0 : 1;
    }

    CHECK(ZstdHelper::IsInitialized());
    CHECK(Compression::HasDecompressor(CompressionAlgorithm::Zstd));

    TestVectors();
    TestFailureModes();

    if (g_failures == 0) std::printf("ALL PASSED\n");
    else std::printf("%d FAILURE(S)\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
