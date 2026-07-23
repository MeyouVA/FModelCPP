// Tests the compression codecs the port ships: the built-in DEFLATE inflater (raw / zlib / gzip), the LZ4
// block decoder, and their registration into the Compression registry. Golden zlib/gzip/deflate vectors were
// produced with .NET's System.IO.Compression; the LZ4 blocks are hand-encoded from the block format.
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "Compression/Compression.h"
#include "Compression/Inflate.h"
#include "Compression/LZ4.h"
#include "Compression/OodleHelper.h"
#include "Compression/ZstdHelper.h"

using namespace CUE4Parse::Compression;

static int g_failures = 0;
#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::cerr << "FAIL: " << #cond << " (line " << __LINE__ << ")\n";  \
            ++g_failures;                                                      \
        }                                                                      \
    } while (0)

// "The quick brown fox jumps over the lazy dog. " x6 == 270 bytes (the source of the golden vectors below).
static std::string Expected()
{
    std::string phrase = "The quick brown fox jumps over the lazy dog. ";
    std::string s;
    for (int i = 0; i < 6; i++) s += phrase;
    return s;
}

static bool Equals(const std::vector<uint8_t>& got, const std::string& want)
{
    if (got.size() != want.size()) return false;
    for (size_t i = 0; i < want.size(); i++)
        if (got[i] != static_cast<uint8_t>(want[i])) return false;
    return true;
}

// Golden data (from .NET System.IO.Compression), original length 270.
static const std::vector<uint8_t> kDeflate = {
    0x0b, 0xc9, 0x48, 0x55, 0x28, 0x2c, 0xcd, 0x4c, 0xce, 0x56, 0x48, 0x2a, 0xca, 0x2f, 0xcf, 0x53,
    0x48, 0xcb, 0xaf, 0x50, 0xc8, 0x2a, 0xcd, 0x2d, 0x28, 0x56, 0xc8, 0x2f, 0x4b, 0x2d, 0x52, 0x28,
    0x01, 0x4a, 0xe7, 0x24, 0x56, 0x55, 0x2a, 0xa4, 0xe4, 0xa7, 0xeb, 0x29, 0x84, 0x0c, 0x77, 0xc5, 0x00
};
static const std::vector<uint8_t> kGzip = {
    0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x0b, 0xc9, 0x48, 0x55, 0x28, 0x2c,
    0xcd, 0x4c, 0xce, 0x56, 0x48, 0x2a, 0xca, 0x2f, 0xcf, 0x53, 0x48, 0xcb, 0xaf, 0x50, 0xc8, 0x2a,
    0xcd, 0x2d, 0x28, 0x56, 0xc8, 0x2f, 0x4b, 0x2d, 0x52, 0x28, 0x01, 0x4a, 0xe7, 0x24, 0x56, 0x55,
    0x2a, 0xa4, 0xe4, 0xa7, 0xeb, 0x29, 0x84, 0x0c, 0x77, 0xc5, 0x00, 0xb5, 0x95, 0xb8, 0xf8, 0x0e,
    0x01, 0x00, 0x00
};
static const std::vector<uint8_t> kZlib = {
    0x78, 0x9c, 0x0b, 0xc9, 0x48, 0x55, 0x28, 0x2c, 0xcd, 0x4c, 0xce, 0x56, 0x48, 0x2a, 0xca, 0x2f,
    0xcf, 0x53, 0x48, 0xcb, 0xaf, 0x50, 0xc8, 0x2a, 0xcd, 0x2d, 0x28, 0x56, 0xc8, 0x2f, 0x4b, 0x2d,
    0x52, 0x28, 0x01, 0x4a, 0xe7, 0x24, 0x56, 0x55, 0x2a, 0xa4, 0xe4, 0xa7, 0xeb, 0x29, 0x84, 0x0c,
    0x77, 0xc5, 0x00, 0x00, 0x00, 0x00, 0x00
};

int main()
{
    const std::string expected = Expected();
    CHECK(expected.size() == 270);

    // ---------- built-in DEFLATE inflater (raw / zlib / gzip) ----------
    {
        std::vector<uint8_t> out(expected.size());
        CHECK(Inflate(kDeflate.data(), (int)kDeflate.size(), out.data(), (int)out.size()) == 270);
        CHECK(Equals(out, expected));
    }
    {
        std::vector<uint8_t> out(expected.size());
        CHECK(ZlibDecompress(kZlib.data(), (int)kZlib.size(), out.data(), (int)out.size()) == 270);
        CHECK(Equals(out, expected));
    }
    {
        std::vector<uint8_t> out(expected.size());
        CHECK(GzipDecompress(kGzip.data(), (int)kGzip.size(), out.data(), (int)out.size()) == 270);
        CHECK(Equals(out, expected));
    }

    // ---------- through the Compression registry (validates the built-in registration) ----------
    CHECK(Equals(Compression::Decompress(kZlib, 270, CompressionMethod::Zlib), expected));
    CHECK(Equals(Compression::Decompress(kGzip, 270, CompressionMethod::Gzip), expected));

    // ---------- LZ4 block decoder ----------
    {
        // Literal-only block: token 0x50 (5 literals), "Hello".
        const std::vector<uint8_t> block = {0x50, 'H', 'e', 'l', 'l', 'o'};
        std::vector<uint8_t> out(5);
        CHECK(LZ4_decompress_safe(block.data(), out.data(), (int)block.size(), (int)out.size()) == 5);
        CHECK(Equals(out, "Hello"));
    }
    {
        // Literals "ABC" + match (offset 3, len 6) => "ABCABCABC", then literals "xyz".
        const std::vector<uint8_t> block = {0x32, 'A', 'B', 'C', 0x03, 0x00, 0x30, 'x', 'y', 'z'};
        std::vector<uint8_t> out(12);
        CHECK(LZ4_decompress_safe(block.data(), out.data(), (int)block.size(), (int)out.size()) == 12);
        CHECK(Equals(out, "ABCABCABCxyz"));
        // ...and through the registry.
        CHECK(Equals(Compression::Decompress(block, 12, CompressionMethod::LZ4), "ABCABCABCxyz"));
    }

    // ---------- None (straight copy) ----------
    {
        std::vector<uint8_t> raw = {1, 2, 3, 4, 5};
        auto out = Compression::Decompress(raw, 5, CompressionMethod::None);
        CHECK(out == raw);
    }

    // ---------- registry state: built-ins registered, native-only codecs are not ----------
    CHECK(Compression::HasDecompressor(CompressionAlgorithm::None));
    CHECK(Compression::HasDecompressor(CompressionAlgorithm::LZ4));
    CHECK(Compression::HasDecompressor(CompressionAlgorithm::Zlib));
    CHECK(Compression::HasDecompressor(CompressionAlgorithm::Gzip));
    CHECK(!Compression::HasDecompressor(CompressionAlgorithm::Oodle)); // needs OodleHelper::Initialize + a DLL
    CHECK(!Compression::HasDecompressor(CompressionAlgorithm::Zstd));  // needs ZstdHelper::Initialize + a DLL
    CHECK(!OodleHelper::IsInitialized());
    CHECK(!ZstdHelper::IsInitialized());

    if (g_failures == 0) std::cout << "test_compression_codecs: all checks passed\n";
    return g_failures;
}
