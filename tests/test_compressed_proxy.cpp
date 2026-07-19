// Round-trip test for FArchive::SerializeCompressedNew via FArchiveLoadCompressedProxy.
// Uses CompressionMethod::None (a plain copy) so no external codec is needed: each chunk's
// "compressed" bytes are identical to its decompressed bytes.
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

#include "UE4/Readers/FArchiveLoadCompressedProxy.h"
#include "UE4/Objects/UObject/FPackageFileSummary.h"

using namespace CUE4Parse::UE4::Readers;
using Summary = CUE4Parse::UE4::Objects::UObject::FPackageFileSummary;

static int g_failures = 0;
#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::cerr << "FAIL: " << #cond << " (line " << __LINE__ << ")\n";  \
            ++g_failures;                                                      \
        }                                                                      \
    } while (0)

static void AppendI64(std::vector<uint8_t>& buf, int64_t value)
{
    uint8_t tmp[8];
    std::memcpy(tmp, &value, 8);
    buf.insert(buf.end(), tmp, tmp + 8);
}

// Builds a v1 (non-swapped) SerializeCompressedNew blob for a single chunk of `payload`, method None.
static std::vector<uint8_t> BuildBlob(const std::vector<uint8_t>& payload)
{
    const int64_t n = static_cast<int64_t>(payload.size());
    std::vector<uint8_t> blob;
    // packageFileTag: CompressedSize == PACKAGE_FILE_TAG marks a valid v1 header;
    // UncompressedSize == PACKAGE_FILE_TAG makes the loader use LOADING_COMPRESSION_CHUNK_SIZE.
    AppendI64(blob, static_cast<int64_t>(Summary::PACKAGE_FILE_TAG));
    AppendI64(blob, static_cast<int64_t>(Summary::PACKAGE_FILE_TAG));
    // summary: total compressed / uncompressed sizes.
    AppendI64(blob, n);
    AppendI64(blob, n);
    // single chunk info.
    AppendI64(blob, n);
    AppendI64(blob, n);
    // chunk payload (compressed == uncompressed for method None).
    blob.insert(blob.end(), payload.begin(), payload.end());
    return blob;
}

int main()
{
    std::vector<uint8_t> payload{10, 20, 30, 40, 50, 60, 70, 80, 90, 100};

    // Full read.
    {
        FArchiveLoadCompressedProxy proxy("blob", BuildBlob(payload), "None");
        auto got = proxy.ReadBytes(static_cast<int>(payload.size()));
        CHECK(got == payload);
        CHECK(proxy.Position == static_cast<int64_t>(payload.size()));
    }

    // Typed reads through the decompressed stream (int32 little-endian: 10 | 20<<8 | 30<<16 | 40<<24).
    {
        FArchiveLoadCompressedProxy proxy("blob", BuildBlob(payload), "None");
        const uint32_t first = proxy.Read<uint32_t>();
        CHECK(first == (10u | (20u << 8) | (30u << 16) | (40u << 24)));
        CHECK(proxy.Position == 4);
    }

    // Forward seek then read the tail.
    {
        FArchiveLoadCompressedProxy proxy("blob", BuildBlob(payload), "None");
        proxy.Seek(5, ESeekOrigin::Begin);
        CHECK(proxy.Position == 5);
        auto tail = proxy.ReadBytes(5);
        std::vector<uint8_t> expected(payload.begin() + 5, payload.end());
        CHECK(tail == expected);
    }

    if (g_failures == 0)
    {
        std::cout << "All SerializeCompressedNew / proxy tests passed.\n";
        return 0;
    }
    std::cout << g_failures << " check(s) failed.\n";
    return 1;
}
