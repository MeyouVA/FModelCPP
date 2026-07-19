// Tests for the additional readers: FPointerArchive, FArchiveBigEndian, FStreamArchive,
// FRandomAccessFileStreamArchive.
#include "UE4/Readers/FByteArchive.h"
#include "UE4/Readers/FPointerArchive.h"
#include "UE4/Readers/FArchiveBigEndian.h"
#include "UE4/Readers/FStreamArchive.h"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace CUE4Parse::UE4::Readers;
using namespace CUE4Parse::UE4::Versions;

static int g_failures = 0;
#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::cerr << "FAIL: " << #cond << " (line " << __LINE__ << ")\n";  \
            ++g_failures;                                                      \
        }                                                                      \
    } while (0)

template <typename T>
static void AppendLE(std::vector<uint8_t>& buf, T value)
{
    uint8_t tmp[sizeof(T)];
    std::memcpy(tmp, &value, sizeof(T));
    buf.insert(buf.end(), tmp, tmp + sizeof(T));
}

int main()
{
    // ---- FPointerArchive ----
    {
        std::vector<uint8_t> storage;
        AppendLE<int32_t>(storage, 0x0BADF00D);
        AppendLE<float>(storage, 3.5f);
        AppendLE<int32_t>(storage, 2);      // array count
        AppendLE<int32_t>(storage, 111);
        AppendLE<int32_t>(storage, 222);

        FPointerArchive ar("ptr", storage.data(), static_cast<int64_t>(storage.size()));
        CHECK(ar.Length == static_cast<int64_t>(storage.size()));
        CHECK(ar.Read<int32_t>() == 0x0BADF00D);
        CHECK(ar.Read<float>() == 3.5f);
        auto arr = ar.ReadArrayCounted<int32_t>();
        CHECK(arr.size() == 2 && arr[0] == 111 && arr[1] == 222);
        CHECK(ar.Position == ar.Length);
    }

    // ---- FArchiveBigEndian (wrapping a byte archive of big-endian data) ----
    {
        std::vector<uint8_t> be;
        // int32 0x12345678, big-endian on disk
        be.insert(be.end(), {0x12, 0x34, 0x56, 0x78});
        // uint16[3] = {0x0102, 0x0304, 0x0506}
        be.insert(be.end(), {0x01, 0x02, 0x03, 0x04, 0x05, 0x06});
        // BE ReadString: int32 count = 5 (big-endian), then "Hello"
        be.insert(be.end(), {0x00, 0x00, 0x00, 0x05, 'H', 'e', 'l', 'l', 'o'});

        FByteArchive base("be-base", be, VersionContainer(GAME_UE4_27));
        FArchiveBigEndian ar(&base);

        CHECK(ar.Name() == "be-base");
        CHECK(ar.Read<int32_t>() == 0x12345678); // swapped from BE

        auto shorts = ar.ReadArray<uint16_t>(3);  // routes through ReadElements -> per-element swap
        CHECK(shorts.size() == 3);
        CHECK(shorts[0] == 0x0102 && shorts[1] == 0x0304 && shorts[2] == 0x0506);

        CHECK(ar.ReadString() == "Hello");
        CHECK(ar.Position == ar.Length);
    }

    // ---- FStreamArchive (over an in-memory std::stringstream) ----
    {
        std::vector<uint8_t> data;
        AppendLE<int32_t>(data, 42);
        AppendLE<double>(data, 6.25);
        auto ss = std::make_shared<std::stringstream>();
        ss->write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));

        FStreamArchive ar("stream", ss);
        CHECK(ar.Length == static_cast<int64_t>(data.size()));
        CHECK(ar.Read<int32_t>() == 42);
        CHECK(ar.Read<double>() == 6.25);

        // Seek back and re-read.
        ar.Seek(0, ESeekOrigin::Begin);
        CHECK(ar.Read<int32_t>() == 42);
    }

    // ---- FRandomAccessFileStreamArchive (over a real temp file) ----
    {
        std::vector<uint8_t> data;
        AppendLE<int32_t>(data, 0x11223344);
        AppendLE<int32_t>(data, 0x55667788);

        auto path = std::filesystem::temp_directory_path() / "fmodelcpp_reader_test.bin";
        {
            std::ofstream out(path, std::ios::binary);
            out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
        }

        {
            FRandomAccessFileStreamArchive ar(path.string());
            CHECK(ar.IsOpen());
            CHECK(ar.Length == static_cast<int64_t>(data.size()));
            CHECK(ar.Read<int32_t>() == 0x11223344);

            // ReadAt should not disturb the sequential Position.
            std::vector<uint8_t> buf(4);
            ar.ReadAt(0, buf.data(), 0, 4);
            CHECK(buf[0] == 0x44 && buf[3] == 0x11);
            CHECK(ar.Read<int32_t>() == 0x55667788);
        }
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }

    if (g_failures == 0)
    {
        std::cout << "All additional-reader tests passed.\n";
        return 0;
    }
    std::cerr << g_failures << " test(s) failed.\n";
    return 1;
}
