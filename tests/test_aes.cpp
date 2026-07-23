// Tests for the Encryption/Aes layer.
//
// The AES-256 inverse cipher here is hand-written (C# gets it from System.Security.Cryptography), so it is
// checked against published vectors rather than against itself: the FIPS-197 §C.3 AES-256 example and the
// four-block NIST SP 800-38A ECB-AES256.Decrypt vector. Those two together exercise the key schedule, all
// fourteen rounds, and block independence in ECB.
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

#include "Encryption/Aes/Aes.h"
#include "Encryption/Aes/FAesKey.h"

using CUE4Parse::Encryption::Aes::Aes;
using CUE4Parse::Encryption::Aes::FAesKey;

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL: %s (line %d)\n", #cond, __LINE__); ++g_failures; } } while (0)

static std::vector<uint8_t> Hex(const std::string& hex)
{
    std::vector<uint8_t> out;
    for (size_t i = 0; i + 1 < hex.size(); i += 2)
    {
        auto nibble = [](char c) -> uint8_t {
            if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
            if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(c - 'a' + 10);
            return static_cast<uint8_t>(c - 'A' + 10);
        };
        out.push_back(static_cast<uint8_t>((nibble(hex[i]) << 4) | nibble(hex[i + 1])));
    }
    return out;
}

static std::string ToHex(const std::vector<uint8_t>& bytes)
{
    static const char* d = "0123456789abcdef";
    std::string s;
    for (uint8_t b : bytes) { s.push_back(d[b >> 4]); s.push_back(d[b & 0xF]); }
    return s;
}

static void TestFAesKey()
{
    // 64 hex digits, no prefix.
    const std::string bare = "0102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F20";
    const FAesKey fromBare(bare);
    CHECK(fromBare.Key.size() == 32);
    CHECK(fromBare.Key[0] == 0x01);
    CHECK(fromBare.Key[31] == 0x20);

    // 66 characters with the 0x prefix — the form FModel stores in AppSettings.
    const FAesKey fromPrefixed("0x" + bare);
    CHECK(fromPrefixed.Key == fromBare.Key);

    // KeyString round-trips, uppercase like Convert.ToHexString.
    CHECK(fromBare.KeyString() == "0x" + bare);
    CHECK(fromBare.ToString() == fromBare.KeyString());
    CHECK(!fromBare.IsDefault());

    const FAesKey zero(std::vector<uint8_t>(32, 0));
    CHECK(zero.IsDefault());
    CHECK(zero.KeyString() == "0x" + std::string(64, '0'));

    // Wrong lengths are rejected, in both the byte and the string form.
    bool threw = false;
    try { FAesKey short_(std::vector<uint8_t>(16, 0)); } catch (const std::invalid_argument&) { threw = true; }
    CHECK(threw);

    threw = false;
    try { FAesKey short_(std::string("0x1234")); } catch (const std::invalid_argument&) { threw = true; }
    CHECK(threw);

    // ...unless the caller opts out, which the IO-store layer needs for non-32-byte keys.
    bool ok = true;
    try { FAesKey ignored(std::vector<uint8_t>(16, 0), true); } catch (const std::exception&) { ok = false; }
    CHECK(ok);

    // A 0x-prefixed string of the wrong total length is rejected even though the hex part parses.
    threw = false;
    try { FAesKey bad(std::string("0x" + bare + "00")); } catch (const std::invalid_argument&) { threw = true; }
    CHECK(threw);
}

static void TestFips197Vector()
{
    // FIPS-197 Appendix C.3 (AES-256).
    const FAesKey key(Hex("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f"));
    const std::vector<uint8_t> cipher = Hex("8ea2b7ca516745bfeafc49904b496089");
    const std::vector<uint8_t> plain = Aes::Decrypt(cipher, key);
    CHECK(ToHex(plain) == "00112233445566778899aabbccddeeff");
}

static void TestNistEcbVector()
{
    // NIST SP 800-38A F.1.6 ECB-AES256.Decrypt — four blocks, which also proves ECB treats blocks
    // independently (no chaining state leaks between them).
    const FAesKey key(Hex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4"));
    const std::vector<uint8_t> cipher = Hex(
        "f3eed1bdb5d2a03c064b5a7e3db181f8"
        "591ccb10d410ed26dc5ba74a31362870"
        "b6ed21b99ca6f4f9f153e7b1beafed1d"
        "23304b7a39f9f3ff067d8d8f9e24ecc7");
    const std::vector<uint8_t> plain = Aes::Decrypt(cipher, key);
    CHECK(ToHex(plain) ==
          "6bc1bee22e409f96e93d7e117393172a"
          "ae2d8a571e03ac9c9eb76fac45af8e51"
          "30c81c46a35ce411e5fbc1191a0a52ef"
          "f69f2445df4f9b17ad2b417be66c3710");

    // Each block decrypts to the same thing on its own as it does inside the run.
    const std::vector<uint8_t> third = Aes::Decrypt(cipher, 32, 16, key);
    CHECK(ToHex(third) == "30c81c46a35ce411e5fbc1191a0a52ef");

    // ...and in place, which is what the extract path uses.
    std::vector<uint8_t> scratch = cipher;
    Aes::DecryptInPlace(scratch.data(), static_cast<int>(scratch.size()), key);
    CHECK(scratch == plain);
}

static void TestRejectsPartialBlocks()
{
    const FAesKey key(std::vector<uint8_t>(32, 0x11));

    // PaddingMode.None: .NET refuses a trailing partial block instead of padding it out.
    bool threw = false;
    try { Aes::Decrypt(std::vector<uint8_t>(17, 0), key); } catch (const std::invalid_argument&) { threw = true; }
    CHECK(threw);

    // An empty buffer is a whole number of blocks (zero of them) and is accepted.
    bool ok = true;
    try { CHECK(Aes::Decrypt(std::vector<uint8_t>(), key).empty()); } catch (const std::exception&) { ok = false; }
    CHECK(ok);

    // Out-of-range windows are rejected rather than read past the end.
    threw = false;
    try { Aes::Decrypt(std::vector<uint8_t>(16, 0), 8, 16, key); } catch (const std::invalid_argument&) { threw = true; }
    CHECK(threw);

    // A key of the wrong length reaches the schedule and is rejected there.
    threw = false;
    try { Aes::Decrypt(std::vector<uint8_t>(16, 0), FAesKey(std::vector<uint8_t>(16, 0), true)); }
    catch (const std::invalid_argument&) { threw = true; }
    CHECK(threw);
}

static void TestBlockSizeConstants()
{
    // C#'s BLOCK_SIZE is in bits (it feeds AesProvider.BlockSize); ALIGN is the byte count.
    CHECK(Aes::ALIGN == 16);
    CHECK(Aes::BLOCK_SIZE == 128);
}

int main()
{
    TestFAesKey();
    TestFips197Vector();
    TestNistEcbVector();
    TestRejectsPartialBlocks();
    TestBlockSizeConstants();

    if (g_failures == 0) std::printf("test_aes: all checks passed\n");
    else std::printf("test_aes: %d check(s) failed\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
