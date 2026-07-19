// Smoke tests for the CUE4Parse C++ reader foundation.
// Exercises FByteArchive + FArchive primitives and the version subsystem.
#include "UE4/Readers/FByteArchive.h"
#include "UE4/Versions/EGame.h"

#include <cstdint>
#include <cstring>
#include <iostream>
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

static void AppendBytes(std::vector<uint8_t>& buf, std::initializer_list<uint8_t> bytes)
{
    buf.insert(buf.end(), bytes);
}

int main()
{
    // --- Build a buffer with a known layout ---
    std::vector<uint8_t> buf;
    AppendLE<int32_t>(buf, 0x12345678);          // int32
    AppendLE<float>(buf, 1.5f);                   // float
    AppendLE<uint8_t>(buf, 1);                    // flag (true)

    // ANSI FString "Hello" -> length 6 (incl null terminator)
    AppendLE<int32_t>(buf, 6);
    AppendBytes(buf, {'H', 'e', 'l', 'l', 'o', 0});

    // UCS2 FString "Hi" -> length -3 (incl null terminator), UTF-16LE units
    AppendLE<int32_t>(buf, -3);
    AppendBytes(buf, {'H', 0, 'i', 0, 0, 0});

    // Count-prefixed int32 array [10, 20, 30]
    AppendLE<int32_t>(buf, 3);
    AppendLE<int32_t>(buf, 10);
    AppendLE<int32_t>(buf, 20);
    AppendLE<int32_t>(buf, 30);

    FByteArchive ar("test", buf);

    CHECK(ar.Name() == "test");
    CHECK(ar.Length == static_cast<int64_t>(buf.size()));
    CHECK(ar.Position == 0);

    CHECK(ar.Read<int32_t>() == 0x12345678);
    CHECK(ar.Read<float>() == 1.5f);
    CHECK(ar.ReadFlag() == true);
    CHECK(ar.ReadFString() == "Hello");
    CHECK(ar.ReadFString() == "Hi");

    auto arr = ar.ReadArrayCounted<int32_t>();
    CHECK(arr.size() == 3);
    CHECK(arr[0] == 10 && arr[1] == 20 && arr[2] == 30);

    CHECK(ar.Position == ar.Length); // consumed everything

    // --- Seek / clone ---
    ar.Seek(0, ESeekOrigin::Begin);
    CHECK(ar.Position == 0);
    CHECK(ar.Read<int32_t>() == 0x12345678);
    auto clone = ar.Clone();
    CHECK(clone->Position == ar.Position);
    CHECK(clone->Read<float>() == 1.5f);
    CHECK(ar.Read<float>() == 1.5f); // original unaffected by clone's read

    // --- Version subsystem ---
    VersionContainer vc(GAME_UE4_LATEST);
    CHECK(vc.Game() == GAME_UE4_LATEST);
    // UE4 latest -> UE4 automatic version, UE5 version 0
    CHECK(vc.Ver().FileVersionUE5 == 0);
    CHECK(vc.Ver().FileVersionUE4 == static_cast<int32_t>(EUnrealEngineObjectUE4Version::AUTOMATIC_VERSION));

    VersionContainer vc5(GAME_UE5_3);
    // GAME_UE5_3 falls in the "< GAME_UE5_4 => (522, 1009)" bucket
    CHECK(vc5.Ver().FileVersionUE4 == 522);
    CHECK(vc5.Ver().FileVersionUE5 == 1009);

    // Explicitly pinned version should be honored as-is.
    VersionContainer vcExplicit(GAME_UE5_3, ETexturePlatform::DesktopMobile, FPackageFileVersion(500, 1005));
    CHECK(vcExplicit.bExplicitVer);
    CHECK(vcExplicit.Ver().FileVersionUE4 == 500);
    CHECK(vcExplicit.Ver().FileVersionUE5 == 1005);

    // --- ReadFReal depends on the version (double for LARGE_WORLD_COORDINATES+) ---
    {
        std::vector<uint8_t> rbuf;
        AppendLE<double>(rbuf, 2.25); // UE5 reads a double
        FByteArchive rar("real", rbuf, VersionContainer(GAME_UE5_3));
        CHECK(rar.ReadFReal() == 2.25f);
    }
    {
        std::vector<uint8_t> rbuf;
        AppendLE<float>(rbuf, 2.25f); // UE4 reads a float
        FByteArchive rar("real", rbuf, VersionContainer(GAME_UE4_27));
        CHECK(rar.ReadFReal() == 2.25f);
    }

    if (g_failures == 0)
    {
        std::cout << "All reader foundation tests passed.\n";
        return 0;
    }
    std::cerr << g_failures << " test(s) failed.\n";
    return 1;
}
