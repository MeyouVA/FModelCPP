// Tests for FMemoryImageArchive — the reader for UE's "frozen" structures.
//
// Everything here is a hand-laid-out memory image, because that is the only way to pin down the one thing
// this reader has to get right: a container's header sits at one position while its payload sits at
// (header position + a relative pointer), and after reading the payload the archive must be back just past
// the header, not past the payload. Each test therefore asserts the resulting Position as well as the data.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "UE4/Readers/FByteArchive.h"
#include "UE4/Readers/FMemoryImageArchive.h"

using namespace CUE4Parse::UE4::Readers;
using CUE4Parse::UE4::Versions::VersionContainer;

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL: %s (line %d)\n", #cond, __LINE__); ++g_failures; } } while (0)

// A fixed-size scratch buffer written by absolute offset, so each test reads like the layout diagram it is.
struct Image
{
    std::vector<uint8_t> Bytes;

    explicit Image(size_t size) : Bytes(size, 0) {}

    template <typename T>
    void Put(size_t offset, T value) { std::memcpy(Bytes.data() + offset, &value, sizeof(T)); }

    // A pre-UE5 frozen pointer: the offset is stored relative to the pointer's own position, shifted left
    // by one to make room for the "is frozen" bit.
    void PutPtr(size_t at, int64_t target)
    {
        Put<uint64_t>(at, static_cast<uint64_t>((target - static_cast<int64_t>(at)) << 1));
    }

    void PutUtf16(size_t offset, const std::string& ascii)
    {
        for (size_t i = 0; i < ascii.size(); ++i)
            Put<uint16_t>(offset + i * 2, static_cast<uint16_t>(static_cast<unsigned char>(ascii[i])));
        Put<uint16_t>(offset + ascii.size() * 2, 0);
    }
};

static std::unique_ptr<FMemoryImageArchive> Open(const Image& image, int arrayAlign = 4,
                                                 CUE4Parse::UE4::Versions::EGame game = CUE4Parse::UE4::Versions::GAME_UE4_LATEST)
{
    auto inner = std::make_shared<FByteArchive>("frozen", image.Bytes, VersionContainer(game));
    return std::make_unique<FMemoryImageArchive>(inner, arrayAlign);
}

static void TestFrozenPointer()
{
    // Pre-UE5: the whole 64-bit word is offset<<1 with bit 0 flagging frozen-ness, and there is no type index.
    Image image(32);
    image.Put<uint64_t>(0, (static_cast<uint64_t>(48) << 1) | 1u);
    auto ar = Open(image);
    const FFrozenMemoryImagePtr ptr(*ar);
    CHECK(ptr.IsFrozen);
    CHECK(ptr.OffsetFromThis == 48);
    CHECK(ptr.TypeIndex == -1);
    CHECK(ar->Position == 8);

    // UE5 repacks it: the offset moves to the top 40 bits and a type index appears in between.
    Image ue5(32);
    ue5.Put<uint64_t>(0, (static_cast<uint64_t>(7) << 24) | (static_cast<uint64_t>(4 + 1) << 1) | 1u);
    auto ar5 = Open(ue5, 4, CUE4Parse::UE4::Versions::GAME_UE5_0);
    const FFrozenMemoryImagePtr ptr5(*ar5);
    CHECK(ptr5.IsFrozen);
    CHECK(ptr5.OffsetFromThis == 7);
    CHECK(ptr5.TypeIndex == 4);

    // A negative offset (the payload sits *before* the pointer) survives the arithmetic shift.
    Image back(32);
    back.Put<uint64_t>(0, static_cast<uint64_t>(static_cast<int64_t>(-24) << 1));
    auto arBack = Open(back);
    const FFrozenMemoryImagePtr ptrBack(*arBack);
    CHECK(!ptrBack.IsFrozen);
    CHECK(ptrBack.OffsetFromThis == -24);
}

static void TestFrozenArray()
{
    //   0  frozen pointer -> 16
    //   8  Num = 3
    //  12  Max = 3
    //  16  payload: 10, 20, 30
    Image image(32);
    image.PutPtr(0, 16);
    image.Put<int32_t>(8, 3);
    image.Put<int32_t>(12, 3);
    image.Put<int32_t>(16, 10);
    image.Put<int32_t>(20, 20);
    image.Put<int32_t>(24, 30);

    auto ar = Open(image);
    const std::vector<int32_t> values = ar->ReadArrayCounted<int32_t>();
    CHECK(values.size() == 3);
    CHECK(values[0] == 10 && values[1] == 20 && values[2] == 30);
    // The payload is elsewhere, so the archive must resume right after the 16-byte header.
    CHECK(ar->Position == 16);

    // An empty array consumes the header and nothing else.
    Image empty(16);
    empty.PutPtr(0, 16);
    auto arEmpty = Open(empty);
    CHECK(arEmpty->ReadArrayCounted<int32_t>().empty());
    CHECK(arEmpty->Position == 16);

    // Num != Max is a corrupt image and must not be read past.
    Image mismatch(32);
    mismatch.PutPtr(0, 16);
    mismatch.Put<int32_t>(8, 3);
    mismatch.Put<int32_t>(12, 4);
    auto arBad = Open(mismatch);
    bool threw = false;
    try { arBad->ReadArrayCounted<int32_t>(); }
    catch (const CUE4Parse::UE4::Exceptions::ParserException&) { threw = true; }
    CHECK(threw);
}

static void TestFrozenArrayWithGetter()
{
    // Elements are realigned to ArrayAlign after each one, so a 1-byte getter still strides by 4.
    //  16  0xAA . . .   20  0xBB . . .   24  0xCC
    Image image(32);
    image.PutPtr(0, 16);
    image.Put<int32_t>(8, 3);
    image.Put<int32_t>(12, 3);
    image.Put<uint8_t>(16, 0xAA);
    image.Put<uint8_t>(20, 0xBB);
    image.Put<uint8_t>(24, 0xCC);

    auto ar = Open(image, 4);
    FMemoryImageArchive& mem = *ar;
    const std::vector<uint8_t> values = mem.ReadArrayWith([&] { return mem.Read<uint8_t>(); });
    CHECK(values.size() == 3);
    CHECK(values[0] == 0xAA && values[1] == 0xBB && values[2] == 0xCC);
    CHECK(mem.Position == 16);

    // With realignment off the same bytes are read back-to-back instead.
    auto ar2 = Open(image, 4);
    FMemoryImageArchive& mem2 = *ar2;
    const std::vector<uint8_t> packed = mem2.ReadArrayWith([&] { return mem2.Read<uint8_t>(); }, false);
    CHECK(packed[0] == 0xAA && packed[1] == 0x00 && packed[2] == 0x00);
}

static void TestFrozenString()
{
    //   0  pointer -> 16;  8  Num = 6;  12  Max = 6;  16  "hello\0" as UCS-2
    Image image(32);
    image.PutPtr(0, 16);
    image.Put<int32_t>(8, 6);
    image.Put<int32_t>(12, 6);
    image.PutUtf16(16, "hello");

    auto ar = Open(image);
    CHECK(ar->ReadFString() == "hello");
    CHECK(ar->Position == 16);

    // Num <= 1 means "no string at all" — not even the terminator is read.
    Image blank(16);
    blank.PutPtr(0, 16);
    blank.Put<int32_t>(8, 1);
    blank.Put<int32_t>(12, 1);
    auto arBlank = Open(blank);
    CHECK(arBlank->ReadFString().empty());
    CHECK(arBlank->Position == 16);

    // A missing null terminator is rejected rather than silently returning a truncated string.
    Image unterminated(32);
    unterminated.PutPtr(0, 16);
    unterminated.Put<int32_t>(8, 3);
    unterminated.Put<int32_t>(12, 3);
    unterminated.PutUtf16(16, "abc"); // writes a terminator at 22...
    unterminated.Put<uint16_t>(20, 'c'); // ...which this overwrites, so the last unit is non-zero
    unterminated.Put<uint16_t>(22, 'd');
    auto arBad = Open(unterminated);
    bool threw = false;
    try { arBad->ReadFString(); }
    catch (const CUE4Parse::UE4::Exceptions::ParserException&) { threw = true; }
    CHECK(threw);
}

static void TestFrozenBitArray()
{
    //   0  pointer -> 16;  8  NumBits = 5;  12  MaxBits = 5;  16  one word, 0b10101
    Image image(24);
    image.PutPtr(0, 16);
    image.Put<int32_t>(8, 5);
    image.Put<int32_t>(12, 5);
    image.Put<int32_t>(16, 0b10101);

    auto ar = Open(image);
    const std::vector<bool> bits = ar->ReadTBitArray();
    CHECK(bits.size() == 5);
    CHECK(bits[0] && !bits[1] && bits[2] && !bits[3] && bits[4]);
    CHECK(ar->Position == 16);

    // 33 bits spill into a second word, which is where a DivideAndRoundUp slip would show up.
    Image wide(32);
    wide.PutPtr(0, 16);
    wide.Put<int32_t>(8, 33);
    wide.Put<int32_t>(12, 33);
    wide.Put<int32_t>(16, 0);
    wide.Put<int32_t>(20, 1);
    auto arWide = Open(wide);
    const std::vector<bool> wideBits = arWide->ReadTBitArray();
    CHECK(wideBits.size() == 33);
    CHECK(!wideBits[0] && !wideBits[31] && wideBits[32]);

    Image none(16);
    none.PutPtr(0, 16);
    auto arNone = Open(none);
    CHECK(arNone->ReadTBitArray().empty());
}

static void TestFrozenArrayOfPtrs()
{
    //   0  outer pointer -> 16;  8  Num = 2;  12  Max = 2
    //  16  entry pointer -> 32          24  entry pointer -> 36
    //  32  111                          36  222
    Image image(48);
    image.PutPtr(0, 16);
    image.Put<int32_t>(8, 2);
    image.Put<int32_t>(12, 2);
    image.PutPtr(16, 32);
    image.PutPtr(24, 36);
    image.Put<int32_t>(32, 111);
    image.Put<int32_t>(36, 222);

    auto ar = Open(image);
    FMemoryImageArchive& mem = *ar;
    const std::vector<int32_t> values = mem.ReadArrayOfPtrs([&] { return mem.Read<int32_t>(); });
    CHECK(values.size() == 2);
    CHECK(values[0] == 111 && values[1] == 222);
    CHECK(mem.Position == 16);
}

static void TestFrozenSparseArrayAndSet()
{
    // TSparseArray header is 40 bytes: pointer(8) + Num/Max(8) + allocation TBitArray(16) + free list(8).
    //   0  pointer -> 40;  8  Num = 2;  12  Max = 2
    //  16  allocation bit array: pointer -> 64, NumBits = 2, MaxBits = 2
    //  32  FirstFreeIndex = 0;  36  NumFreeIndices = 0
    //  40  element 0 (stride 8)   48  element 1
    //  64  allocation words: 0b11
    Image image(72);
    image.PutPtr(0, 40);
    image.Put<int32_t>(8, 2);
    image.Put<int32_t>(12, 2);
    image.PutPtr(16, 64);
    image.Put<int32_t>(24, 2);
    image.Put<int32_t>(28, 2);
    image.Put<int32_t>(40, 777);
    image.Put<int32_t>(48, 888);
    image.Put<int32_t>(64, 0b11);

    auto ar = Open(image);
    FMemoryImageArchive& mem = *ar;
    const std::vector<int32_t> values = mem.ReadTSparseArray([&] { return mem.Read<int32_t>(); }, 8);
    CHECK(values.size() == 2);
    CHECK(values[0] == 777 && values[1] == 888);
    CHECK(mem.Position == 40);

    // A cleared allocation bit means the slot is a hole: it is skipped, but still occupies its stride.
    Image holed(72);
    holed.PutPtr(0, 40);
    holed.Put<int32_t>(8, 2);
    holed.Put<int32_t>(12, 2);
    holed.PutPtr(16, 64);
    holed.Put<int32_t>(24, 2);
    holed.Put<int32_t>(28, 2);
    holed.Put<int32_t>(40, 777);
    holed.Put<int32_t>(48, 888);
    holed.Put<int32_t>(64, 0b10); // only slot 1 is live
    auto arHoled = Open(holed);
    FMemoryImageArchive& memHoled = *arHoled;
    const std::vector<int32_t> live = memHoled.ReadTSparseArray([&] { return memHoled.Read<int32_t>(); }, 8);
    CHECK(live.size() == 1);
    CHECK(live[0] == 888);

    // TSet is the same sparse array with 8 extra bytes per element (HashNextId + HashIndex) and a 12-byte
    // hash header after it, so the stride becomes 16 and Position ends 12 further along.
    Image set(96);
    set.PutPtr(0, 40);
    set.Put<int32_t>(8, 2);
    set.Put<int32_t>(12, 2);
    set.PutPtr(16, 80);
    set.Put<int32_t>(24, 2);
    set.Put<int32_t>(28, 2);
    set.Put<int32_t>(40, 5);
    set.Put<int32_t>(56, 6);
    set.Put<int32_t>(80, 0b11);
    auto arSet = Open(set);
    FMemoryImageArchive& memSet = *arSet;
    const std::vector<int32_t> setValues = memSet.ReadTSet([&] { return memSet.Read<int32_t>(); }, 8);
    CHECK(setValues.size() == 2);
    CHECK(setValues[0] == 5 && setValues[1] == 6);
    CHECK(memSet.Position == 52);
}

static void TestPositionStaysInSyncWithInner()
{
    // The archive keeps its own Position and pushes it into the inner reader on every seam; a mismatch
    // would corrupt every relative-pointer jump above.
    Image image(32);
    image.Put<int32_t>(0, 1);
    image.Put<int32_t>(4, 2);

    auto inner = std::make_shared<FByteArchive>("frozen", image.Bytes);
    FMemoryImageArchive mem(inner);
    CHECK(mem.Read<int32_t>() == 1);
    CHECK(mem.Position == 4);
    CHECK(inner->Position == 4);

    mem.Position = 0;
    CHECK(mem.Read<int32_t>() == 1);

    mem.Seek(4, ESeekOrigin::Begin);
    CHECK(mem.Read<int32_t>() == 2);
    CHECK(inner->Position == 8);

    CHECK(mem.Name() == "frozen");
    CHECK(mem.Length == 32);

    // A clone shares nothing but the position it was taken at.
    auto clone = mem.Clone();
    CHECK(clone->Position == 8);
    clone->Position = 0;
    CHECK(mem.Position == 8);
}

int main()
{
    TestFrozenPointer();
    TestFrozenArray();
    TestFrozenArrayWithGetter();
    TestFrozenString();
    TestFrozenBitArray();
    TestFrozenArrayOfPtrs();
    TestFrozenSparseArrayAndSet();
    TestPositionStaysInSyncWithInner();

    if (g_failures == 0) std::printf("test_memory_image: all checks passed\n");
    else std::printf("test_memory_image: %d check(s) failed\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
