// Tests for the Core/Misc value structs: FGuid, FUInt128, FDateTime, FFrameNumber, FFrameRate,
// FSHAHash, FEngineVersionBase / FEngineVersion.
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

#include "UE4/Objects/Core/Misc/FGuid.h"
#include "UE4/Objects/Core/Misc/FDateTime.h"
#include "UE4/Objects/Core/Misc/FFrameNumber.h"
#include "UE4/Objects/Core/Misc/FFrameRate.h"
#include "UE4/Objects/Core/Misc/FSHAHash.h"
#include "UE4/Objects/Core/Misc/FEngineVersion.h"
#include "UE4/Readers/FByteArchive.h"

using namespace CUE4Parse::UE4::Objects::Core::Misc;
using namespace CUE4Parse::UE4::Readers;

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

// Append a length-prefixed ANSI FString (as ReadFString expects: int32 length incl. null, then bytes).
static void AppendFString(std::vector<uint8_t>& buf, const std::string& s)
{
    AppendLE<int32_t>(buf, static_cast<int32_t>(s.size() + 1));
    buf.insert(buf.end(), s.begin(), s.end());
    buf.push_back(0);
}

static void TestFGuid()
{
    // FGuid is a trivially copyable, standard-layout POD so FArchive::Read<FGuid>() works.
    static_assert(std::is_trivially_copyable_v<FGuid>, "FGuid must be trivially copyable");
    static_assert(std::is_standard_layout_v<FGuid>, "FGuid must be standard-layout");
    static_assert(sizeof(FGuid) == 16, "FGuid must be exactly 16 bytes");

    FGuid g(0x12345678u, 0x9ABCDEF0u, 0x0FEDCBA9u, 0x87654321u);
    CHECK(g.ToString() == "123456789ABCDEF00FEDCBA987654321");
    CHECK(g.ToString(EGuidFormats::Digits) == "123456789ABCDEF00FEDCBA987654321");
    CHECK(g.ToString(EGuidFormats::DigitsWithHyphens) == "12345678-9ABC-DEF0-0FED-CBA987654321");
    CHECK(g.ToString(EGuidFormats::DigitsWithHyphensInBraces) == "{12345678-9ABC-DEF0-0FED-CBA987654321}");
    CHECK(g.ToString(EGuidFormats::DigitsWithHyphensInParentheses) == "(12345678-9ABC-DEF0-0FED-CBA987654321)");
    CHECK(g.ToString(EGuidFormats::UniqueObjectGuid) == "12345678-9ABCDEF0-0FEDCBA9-87654321");

    // Round-trip through the hex-string constructor.
    FGuid parsed(g.ToString());
    CHECK(parsed == g);
    CHECK(parsed.A == 0x12345678u && parsed.D == 0x87654321u);

    CHECK(g.IsValid());
    CHECK(!FGuid().IsValid());
    CHECK(FGuid(1u).A == 1u && FGuid(1u).D == 1u);

    // Read<FGuid> reads 4 little-endian uint32s.
    std::vector<uint8_t> buf;
    AppendLE<uint32_t>(buf, 0x11111111u);
    AppendLE<uint32_t>(buf, 0x22222222u);
    AppendLE<uint32_t>(buf, 0x33333333u);
    AppendLE<uint32_t>(buf, 0x44444444u);
    FByteArchive ar("guid", buf);
    FGuid read = ar.Read<FGuid>();
    CHECK(read == FGuid(0x11111111u, 0x22222222u, 0x33333333u, 0x44444444u));
    CHECK(ar.Position == 16);

    // HexValuesInBraces spot check.
    FGuid z(0, 0, 0, 0);
    CHECK(z.ToString(EGuidFormats::HexValuesInBraces) ==
          "{0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}}");

    // Short (url-safe base64, padding trimmed to 22 chars for 16 bytes).
    std::string shortForm = g.ToString(EGuidFormats::Short);
    CHECK(shortForm.size() == 22);
    CHECK(shortForm.find('+') == std::string::npos && shortForm.find('/') == std::string::npos);
}

static void TestFDateTime()
{
    // 0 ticks == 0001-01-01 00:00:00.
    CHECK(FDateTime(0).ToString() == "0001.01.01-00.00.00");
    // 2021-01-01 00:00:00 UTC in .NET ticks.
    // Days from 0001-01-01 to 2021-01-01 = 737790; * ticks/day.
    const int64_t ticks2021 = 737790LL * 864000000000LL;
    CHECK(FDateTime(ticks2021).ToString() == "2021.01.01-00.00.00");
    // Add 13:37:00.
    const int64_t plusTime = ticks2021 + (13LL * 3600 + 37LL * 60) * 10000000LL;
    CHECK(FDateTime(plusTime).ToString() == "2021.01.01-13.37.00");
}

static void TestFrameTypes()
{
    CHECK(FFrameNumber(42).ToString() == "42");
    CHECK(FFrameNumber(3.9f).Value == 3);
    CHECK(FFrameRate(30000, 1001).ToString() == "Numerator: 30000, Denominator: 1001");
}

static void TestFSHAHash()
{
    std::vector<uint8_t> buf;
    for (int i = 0; i < 20; i++) buf.push_back(static_cast<uint8_t>(0xA0 + i));
    buf.push_back(0xFF); // trailing byte (should not be read by the 20-byte ctor)

    FByteArchive ar("sha", buf);
    FSHAHash sha(ar);
    CHECK(ar.Position == 20);
    CHECK(sha.IsValid());
    CHECK(sha.ToString() == "A0A1A2A3A4A5A6A7A8A9AAABACADAEAFB0B1B2B3");

    FSHAHash zero;
    CHECK(!zero.IsValid());
    CHECK(sha != zero);

    // customSize > SIZE: reads 20 bytes then skips the remainder.
    std::vector<uint8_t> buf2(buf.begin(), buf.begin() + 20);
    buf2.resize(24, 0xCC); // 4 extra bytes to skip
    FByteArchive ar2("sha2", buf2);
    FSHAHash sha2(ar2, 24);
    CHECK(ar2.Position == 24);
    CHECK(sha2.ToString() == sha.ToString());
}

static void TestFEngineVersion()
{
    std::vector<uint8_t> buf;
    AppendLE<uint16_t>(buf, 5);          // Major
    AppendLE<uint16_t>(buf, 3);          // Minor
    AppendLE<uint16_t>(buf, 2);          // Patch
    AppendLE<uint32_t>(buf, 12345678u);  // Changelist
    AppendFString(buf, "++UE5+Release-5.3");

    FByteArchive ar("ver", buf);
    FEngineVersion ver(ar);
    CHECK(ver.Major == 5 && ver.Minor == 3 && ver.Patch == 2);
    CHECK(ver.Changelist() == 12345678u);
    CHECK(!ver.IsEmpty());
    CHECK(ver.HasChangelist());
    // Branch decodes '+' back to '/'.
    CHECK(ver.Branch() == "//UE5/Release-5.3");
    CHECK(ver.ToString(EVersionComponent::Patch) == "5.3.2");
    CHECK(ver.ToString(EVersionComponent::Changelist) == "5.3.2-12345678");

    // Licensee bit handling on the base type.
    FEngineVersionBase lic(1, 0, 0, FEngineVersionBase::EncodeLicenseeChangeList(7));
    CHECK(lic.IsLicenseeVersion());
    CHECK(lic.Changelist() == 7); // top bit masked off
}

int main()
{
    TestFGuid();
    TestFDateTime();
    TestFrameTypes();
    TestFSHAHash();
    TestFEngineVersion();

    if (g_failures == 0)
    {
        std::cout << "All Core/Misc tests passed.\n";
        return 0;
    }
    std::cout << g_failures << " check(s) failed.\n";
    return 1;
}
