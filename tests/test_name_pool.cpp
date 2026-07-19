// Tests for the FName name-pool layer: FMappedName bit-unpacking, FSerializedNameHeader, the two
// FNameEntrySerialized loaders, and FName's name-map / FMappedName constructors and comparisons.
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "UE4/Objects/UObject/FName.h"
#include "UE4/Objects/UObject/FNameEntrySerialized.h"
#include "UE4/IO/Objects/FMappedName.h"
#include "UE4/Readers/FByteArchive.h"

using namespace CUE4Parse::UE4::Objects::UObject;
using namespace CUE4Parse::UE4::Readers;
using CUE4Parse::UE4::IO::Objects::FMappedName;

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

// Length-prefixed ANSI FString: int32 length incl. null terminator, then bytes + null.
static void AppendFString(std::vector<uint8_t>& buf, const std::string& s)
{
    AppendLE<int32_t>(buf, static_cast<int32_t>(s.size() + 1));
    buf.insert(buf.end(), s.begin(), s.end());
    buf.push_back(0);
}

static void AppendBytes(std::vector<uint8_t>& buf, const std::string& s)
{
    buf.insert(buf.end(), s.begin(), s.end());
}

int main()
{
    // --- FMappedName: low 30 bits = index, top 2 bits = type ---
    {
        FMappedName mn;
        mn._nameIndex = (2u << FMappedName::TypeShift) | 42u; // Type = Global (2), NameIndex = 42
        mn.ExtraIndex = 7;
        CHECK(mn.NameIndex() == 42);
        CHECK(mn.Type() == FMappedName::EType::Global);
        CHECK(mn.IsGlobal());

        FMappedName pkg;
        pkg._nameIndex = 5u; // Type = Package (0)
        CHECK(pkg.Type() == FMappedName::EType::Package);
        CHECK(!pkg.IsGlobal());
    }

    // --- FSerializedNameHeader: high bit of byte0 = UTF-16, 15-bit big-endian-ish length ---
    {
        FSerializedNameHeader utf8{0x00, 0x04};
        CHECK(!utf8.IsUtf16());
        CHECK(utf8.Length() == 4);

        FSerializedNameHeader utf16{0x80, 0x03}; // UTF-16, length 3
        CHECK(utf16.IsUtf16());
        CHECK(utf16.Length() == 3);

        FSerializedNameHeader big{0x01, 0x02}; // length = (1<<8)+2 = 258
        CHECK(big.Length() == 258);
    }

    // --- FNameEntrySerialized(FArchive): modern UE4 archive (default game) reads FString + skips 4 hash bytes ---
    {
        std::vector<uint8_t> buf;
        AppendFString(buf, "  Hello  ");  // trimmed to "Hello"
        AppendLE<uint32_t>(buf, 0);        // NAME_HASHES skip (4 bytes)
        FByteArchive ar("names", buf);
        FNameEntrySerialized entry(ar);
        CHECK(entry.Name.has_value() && *entry.Name == "Hello");
        CHECK(entry.ToString() == "Hello");
        CHECK(ar.Position == static_cast<int64_t>(buf.size())); // consumed the hash bytes too
    }

    // --- LoadNameBatch(Ar): IO Store name-batch blob (headers block, then string bytes) ---
    {
        std::vector<uint8_t> buf;
        AppendLE<int32_t>(buf, 2);         // num
        AppendLE<uint32_t>(buf, 6);        // numStringBytes (skipped)
        AppendLE<uint64_t>(buf, 0);        // hashVersion (skipped)
        AppendLE<uint64_t>(buf, 0);        // hash[0] (skipped)
        AppendLE<uint64_t>(buf, 0);        // hash[1] (skipped)
        // headers
        AppendLE<uint8_t>(buf, 0x00); AppendLE<uint8_t>(buf, 0x04); // "Test" (utf8, len 4)
        AppendLE<uint8_t>(buf, 0x00); AppendLE<uint8_t>(buf, 0x02); // "Hi"   (utf8, len 2)
        // strings
        AppendBytes(buf, "Test");
        AppendBytes(buf, "Hi");

        FByteArchive ar("batch", buf);
        auto entries = FNameEntrySerialized::LoadNameBatch(ar);
        CHECK(entries.size() == 2);
        CHECK(entries[0].ToString() == "Test");
        CHECK(entries[1].ToString() == "Hi");
    }

    // --- LoadNameBatch(Ar, count): per-header loader (Latin-1 path) ---
    {
        std::vector<uint8_t> buf;
        AppendLE<uint8_t>(buf, 0x00); AppendLE<uint8_t>(buf, 0x02); AppendBytes(buf, "Ok");
        AppendLE<uint8_t>(buf, 0x00); AppendLE<uint8_t>(buf, 0x03); AppendBytes(buf, "Foo");
        FByteArchive ar("headers", buf);
        auto entries = FNameEntrySerialized::LoadNameBatch(ar, 2);
        CHECK(entries.size() == 2);
        CHECK(entries[0].ToString() == "Ok");
        CHECK(entries[1].ToString() == "Foo");
    }

    // --- FName from a name map + index/number, and via FMappedName ---
    {
        std::vector<FNameEntrySerialized> nameMap{
            FNameEntrySerialized(std::string("Actor")),
            FNameEntrySerialized(std::string("Component")),
            FNameEntrySerialized(std::string("None")),
        };

        FName plain(nameMap, 0, 0);          // "Actor", Index path
        CHECK(plain.Text() == "Actor");
        CHECK(plain.PlainText() == "Actor");
        CHECK(plain.ComparisonMethod == FNameComparisonMethod::Index);
        CHECK(!plain.IsNone());

        FName numbered(nameMap, 1, 3);       // "Component" + number (3 -> "_2")
        CHECK(numbered.Text() == "Component_2");

        FName none(nameMap, 2, 0);
        CHECK(none.IsNone());

        // FMappedName drives (index, extraIndex==number).
        FMappedName mn;
        mn._nameIndex = 1u;                  // NameIndex 1 -> "Component"
        mn.ExtraIndex = 0;
        FName fromMapped(mn, nameMap);
        CHECK(fromMapped.Text() == "Component");
        CHECK(fromMapped.Index == 1);

        // Index-based equality ignores text; two entries with the same index+number are equal.
        FName a(nameMap, 0, 0);
        FName b(nameMap, 0, 0);
        CHECK(a == b);
        FName c(nameMap, 1, 0);
        CHECK(a != c);
        CHECK(a == 0);   // FName == int compares Index
        CHECK(a != 1u);
    }

    // --- Text-based comparison is case-insensitive (runtime OrdinalIgnoreCase) ---
    {
        FName lower(std::string("myactor"));
        FName upper(std::string("MyActor"));
        CHECK(lower.ComparisonMethod == FNameComparisonMethod::Text);
        CHECK(lower == upper);
        CHECK(lower.CompareTo(upper) == 0);

        FName other(std::string("Zeta"));
        CHECK(lower != other);
        CHECK(lower.CompareTo(other) < 0); // 'm'/'M' < 'z'/'Z'
    }

    if (g_failures == 0)
    {
        std::cout << "All FName name-pool tests passed.\n";
        return 0;
    }
    std::cout << g_failures << " check(s) failed.\n";
    return 1;
}
