// Smoke tests for the compression core (Compression + registry) and the standalone Utils
// (CityHash, CRC32, MathUtils, HexUtils, StringUtils, ArrayUtils).
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

#include "Compression/Compression.h"
#include "Compression/CompressionMethod.h"
#include "Utils/ArrayUtils.h"
#include "Utils/CRC32.h"
#include "Utils/CityHash.h"
#include "Utils/HexUtils.h"
#include "Utils/MathUtils.h"
#include "Utils/StringUtils.h"

using namespace CUE4Parse::Compression;
using namespace CUE4Parse::Utils;

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL: %s (line %d)\n", #cond, __LINE__); ++g_failures; } } while (0)

static void TestCompression()
{
    // None => straight copy.
    std::vector<uint8_t> src{1, 2, 3, 4, 5};
    auto out = Compression::Decompress(src, static_cast<int>(src.size()), CompressionMethod::None);
    CHECK(out == src);

    // Unknown/unregistered algorithm throws until a codec is registered.
    bool threw = false;
    try
    {
        std::vector<uint8_t> comp{9, 9, 9};
        Compression::Decompress(comp, 16, CompressionMethod::LZ4);
    }
    catch (const std::exception&) { threw = true; }
    CHECK(threw);
    CHECK(!Compression::HasDecompressor(CompressionAlgorithm::LZ4));

    // Register a trivial "codec" (identity copy) and verify it is invoked and length-checked.
    Compression::RegisterDecompressor(CompressionAlgorithm::LZ4,
        [](const uint8_t* s, int sLen, uint8_t* d, int dLen, int& written) -> bool
        {
            int n = sLen < dLen ? sLen : dLen;
            for (int i = 0; i < n; ++i) d[i] = s[i];
            written = dLen; // pretend we filled the whole output
            return true;
        });
    CHECK(Compression::HasDecompressor(CompressionAlgorithm::LZ4));

    std::vector<uint8_t> comp{10, 20, 30};
    auto dec = Compression::Decompress(comp, 3, CompressionMethod::LZ4);
    CHECK(dec.size() == 3);
    CHECK(dec[0] == 10 && dec[1] == 20 && dec[2] == 30);
}

static void TestCityHash()
{
    std::vector<uint8_t> small{'a', 'b', 'c'};                       // <=16 branch
    std::vector<uint8_t> mid(24, 0x5A);                              // 17-32 branch
    std::vector<uint8_t> big(50, 0x11);                             // 33-64 branch
    std::vector<uint8_t> huge(200);                                  // >64 loop branch
    for (size_t i = 0; i < huge.size(); ++i) huge[i] = static_cast<uint8_t>(i);

    // Deterministic.
    CHECK(CityHash::CityHash64(small) == CityHash::CityHash64(small));
    CHECK(CityHash::CityHash64(huge) == CityHash::CityHash64(huge));

    // Different lengths / content -> different hashes (all branches exercised).
    CHECK(CityHash::CityHash64(small) != CityHash::CityHash64(mid));
    CHECK(CityHash::CityHash64(mid) != CityHash::CityHash64(big));
    CHECK(CityHash::CityHash64(big) != CityHash::CityHash64(huge));

    // Seed relationship: CityHash64WithSeed(b, s) == HashLen16(CityHash64(b)-K2, s) via WithSeeds.
    CHECK(CityHash::CityHash64WithSeed(huge, 12345) == CityHash::CityHash64WithSeeds(huge, 0x9ae16a3b2f90404fULL, 12345));
    CHECK(CityHash::CityHash64WithSeed(huge, 1) != CityHash::CityHash64WithSeed(huge, 2));

    // Empty throws.
    bool threw = false;
    try { CityHash::CityHash64(std::vector<uint8_t>{}); } catch (const std::exception&) { threw = true; }
    CHECK(threw);
}

static void TestCRC32()
{
    // Standard reflected CRC-32 (zip/gzip): CRC32("123456789") == 0xCBF43926.
    CRC32 crc;
    std::string s = "123456789";
    std::vector<uint8_t> data(s.begin(), s.end());
    int32_t result = crc.GetCrc32(data);
    CHECK(static_cast<uint32_t>(result) == 0xCBF43926u);

    // Reset then feed byte-by-byte -> same result.
    crc.Reset();
    for (uint8_t b : data) crc.UpdateCRC(b);
    CHECK(static_cast<uint32_t>(crc.Crc32Result()) == 0xCBF43926u);
    // Reset() clears only the register (not the byte counter) and UpdateCRC(byte) doesn't bump it,
    // so TotalBytesRead stays at the 9 accumulated by the GetCrc32 call above — matching C#.
    CHECK(crc.TotalBytesRead() == 9);
}

static void TestMathUtils()
{
    CHECK(std::fabs(InvSqrt(4.0f) - 0.5f) < 0.01f);
    CHECK(DivideAndRoundUp(10, 3) == 4);
    CHECK(DivideAndRoundUp(9, 3) == 3);
    CHECK(std::fabs(ToDegrees(MathConstants::PI_F) - 180.0f) < 0.001f);
    CHECK(std::fabs(ToRadians(180.0f) - MathConstants::PI_F) < 0.0001f);
    CHECK(Square(3.0f) == 9.0f);
    CHECK(Clamp(15, 0, 10) == 10);
    CHECK(Clamp(-5, 0, 10) == 0);
    CHECK(Clamp(5, 0, 10) == 5);
    CHECK(RoundToInt(2.5f) == 3);
    CHECK(FloorToInt(-1.5f) == -2);
    CHECK(TruncToInt(-1.9f) == -1);

    // Morton round-trip.
    uint32_t v = 0x1234;
    CHECK(ReverseMortonCode2(MortonCode2(v)) == v);
}

static void TestHexUtils()
{
    auto b = ParseHexBinary("deadBEEF");
    CHECK(b.size() == 4);
    CHECK(b[0] == 0xde && b[1] == 0xad && b[2] == 0xbe && b[3] == 0xef);

    bool threw = false;
    try { ParseHexBinary("abc"); } catch (const std::exception&) { threw = true; }
    CHECK(threw);
}

static void TestStringUtils()
{
    CHECK(SubstringBefore(std::string("key=value"), '=') == "key");
    CHECK(SubstringAfter(std::string("key=value"), '=') == "value");
    CHECK(SubstringBefore(std::string("nodelim"), '=') == "nodelim");
    CHECK(SubstringAfterLast(std::string("a/b/c"), '/') == "c");
    CHECK(SubstringBeforeLast(std::string("a/b/c"), '/') == "a/b");
    CHECK(SubstringAfter(std::string("aXXbXXc"), std::string("XX")) == "bXXc");
    CHECK(SubstringAfterLast(std::string("aXXbXXc"), std::string("XX")) == "c");
    CHECK(SubstringAfterWithLast(std::string("a.b.c"), '.') == ".c");
    CHECK(Contains(std::string("hello world"), std::string("o w")));
    CHECK(!Contains(std::string("hello"), std::string("z")));
}

static void TestArrayUtils()
{
    std::vector<uint8_t> a{1, 2, 3, 4, 5};
    auto sub = SubByteArray(a, 3);
    CHECK(sub.size() == 3 && sub[0] == 1 && sub[2] == 3);

    std::vector<bool> bits{false, false, true, false};
    CHECK(Contains(bits, true));
    CHECK(GetOrFalse(bits, 2));
    CHECK(!GetOrFalse(bits, 0));
    CHECK(!GetOrFalse(bits, 99)); // out of range

    std::vector<bool> dst(4, false);
    std::vector<bool> readBits{true, true, false, true};
    SetRangeFromRange(dst, 1, 2, readBits, 0);
    CHECK(dst[0] == false && dst[1] == true && dst[2] == true && dst[3] == false);
}

int main()
{
    TestCompression();
    TestCityHash();
    TestCRC32();
    TestMathUtils();
    TestHexUtils();
    TestStringUtils();
    TestArrayUtils();

    if (g_failures == 0)
    {
        std::printf("All compression/utils tests passed.\n");
        return 0;
    }
    std::printf("%d check(s) failed.\n", g_failures);
    return 1;
}
