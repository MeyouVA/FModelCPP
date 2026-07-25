// Tests the FMOD .bank ("FEV " RIFF) reader: FModReader's chunk walker, the shared "global reader"
// primitives every node leans on, the Objects/Metadata value types, and EventNodesResolver's graph walk.
//
// As with the Wwise suite, including every header at once is a real part of the test -- the FMod tree is
// almost entirely header-only, so a header that nothing includes is never compiled at all. The full include
// list below is what makes the ported tree real, and it doubles as an inventory.
//
// The behavioural half feeds hand-built byte buffers through the readers and pins the things a mechanical
// C#-to-C++ translation gets wrong:
//   * C#'s BinaryReader.ReadBoolean/ReadByte read ONE byte, while FArchive::ReadBoolean reads FOUR. Every
//     bool/byte field in this tree is a single byte, and getting one wrong silently shifts everything after
//     it -- so the synthetic bank below is byte-exact and the chunk walker's `Position != nextNode`
//     corrective seek would hide the damage rather than report it. Checking parsed *values* is what catches it.
//   * ReadX16's high-bit continuation: a plain int16, or 15 low bits plus a following uint16 of high bits.
//   * ReadElemListImp vs ReadVersionedElemListImp -- same count encoding (ReadX16 >> 1), but the uint16
//     payload size appears ONCE before the list vs. once per element.
//   * FModGuid is NOT laid out like FGuid: the conversion rotates B by 16 and byte-reverses C and D.
//   * the version gates, where a field widens, moves, or disappears between bank versions.
//   * FModReader's static per-parse state (FormatInfo/SoundDataInfo/EncryptionKey), which every node reader
//     consults -- it has to be set before constructing a node standalone.
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "UE4/Readers/FByteArchive.h"

#include "UE4/FMod/FModReader.h"
#include "UE4/FMod/FModSoundBank.h"
#include "UE4/FMod/Fsb5Decryption.h"

#include "UE4/FMod/Enums/EAutomationConflictResolutionMethod.h"
#include "UE4/FMod/Enums/EClockSource.h"
#include "UE4/FMod/Enums/EDSPType.h"
#include "UE4/FMod/Enums/EEvaluatorType.h"
#include "UE4/FMod/Enums/EFModStudioParameterType.h"
#include "UE4/FMod/Enums/EFModVersion.h"
#include "UE4/FMod/Enums/EModulatorType.h"
#include "UE4/FMod/Enums/EPlaylistPlayMode.h"
#include "UE4/FMod/Enums/EPlaylistSelectionMode.h"
#include "UE4/FMod/Enums/EPortType.h"
#include "UE4/FMod/Enums/EPropertyType.h"
#include "UE4/FMod/Enums/EQuantizationUnit.h"
#include "UE4/FMod/Enums/ERIFFID.h"
#include "UE4/FMod/Enums/ESpectralSidechainModulatorMode.h"
#include "UE4/FMod/Enums/EStringTableType.h"
#include "UE4/FMod/Enums/EWaveformLoadingMode.h"

#include "UE4/FMod/Metadata/FBankInfo.h"
#include "UE4/FMod/Metadata/FFormatInfo.h"
#include "UE4/FMod/Metadata/FHashInfo.h"
#include "UE4/FMod/Metadata/HashData.h"
#include "UE4/FMod/Metadata/SoundDataInfo.h"
#include "UE4/FMod/Metadata/SoundTable.h"
#include "UE4/FMod/Metadata/StringTable.h"

#include "UE4/FMod/Objects/FControllerOverride.h"
#include "UE4/FMod/Objects/FCurvePoint.h"
#include "UE4/FMod/Objects/FEffectParameter.h"
#include "UE4/FMod/Objects/FEvaluator.h"
#include "UE4/FMod/Objects/FEventParameterStub.h"
#include "UE4/FMod/Objects/FFadeCurve.h"
#include "UE4/FMod/Objects/FLegacyParameterConditions.h"
#include "UE4/FMod/Objects/FLegacyTriggerBox.h"
#include "UE4/FMod/Objects/FMappingPoint.h"
#include "UE4/FMod/Objects/FMixerStrip.h"
#include "UE4/FMod/Objects/FModGuid.h"
#include "UE4/FMod/Objects/FParameterId.h"
#include "UE4/FMod/Objects/FParentContext.h"
#include "UE4/FMod/Objects/FPlaylistEntry.h"
#include "UE4/FMod/Objects/FQuantization.h"
#include "UE4/FMod/Objects/FRadixTreePacked.h"
#include "UE4/FMod/Objects/FRangeFloat.h"
#include "UE4/FMod/Objects/FRoutable.h"
#include "UE4/FMod/Objects/FSnapshot.h"
#include "UE4/FMod/Objects/FSustainPoint.h"
#include "UE4/FMod/Objects/FTimelineNamedMarker.h"
#include "UE4/FMod/Objects/FTimelineTempoMarker.h"
#include "UE4/FMod/Objects/FTriggerBox.h"
#include "UE4/FMod/Objects/FTriggerBoxParameterLayout.h"
#include "UE4/FMod/Objects/FTriggerDelay.h"
#include "UE4/FMod/Objects/FUInt24.h"
#include "UE4/FMod/Objects/FUserPropertyFloat.h"
#include "UE4/FMod/Objects/FUserPropertyString.h"

#include "UE4/FMod/Nodes/ControllerNode.h"
#include "UE4/FMod/Nodes/ControllerOwnerNode.h"
#include "UE4/FMod/Nodes/CurveNode.h"
#include "UE4/FMod/Nodes/EventNode.h"
#include "UE4/FMod/Nodes/MappingNode.h"
#include "UE4/FMod/Nodes/ModulatorNode.h"
#include "UE4/FMod/Nodes/ParameterLayoutNode.h"
#include "UE4/FMod/Nodes/ParameterNode.h"
#include "UE4/FMod/Nodes/PlaylistNode.h"
#include "UE4/FMod/Nodes/PropertyNode.h"
#include "UE4/FMod/Nodes/SnapshotNode.h"
#include "UE4/FMod/Nodes/SoundDataNode.h"
#include "UE4/FMod/Nodes/TimelineNode.h"
#include "UE4/FMod/Nodes/VCANode.h"
#include "UE4/FMod/Nodes/WaveformResourceNode.h"

#include "UE4/FMod/Nodes/Buses/BaseBusNode.h"
#include "UE4/FMod/Nodes/Buses/BusNode.h"
#include "UE4/FMod/Nodes/Buses/GroupBusNode.h"
#include "UE4/FMod/Nodes/Buses/InputBusNode.h"
#include "UE4/FMod/Nodes/Buses/MasterBusNode.h"
#include "UE4/FMod/Nodes/Buses/OutputPortNode.h"
#include "UE4/FMod/Nodes/Buses/ReturnBusNode.h"

#include "UE4/FMod/Nodes/Effects/BaseEffectNode.h"
#include "UE4/FMod/Nodes/Effects/BuiltInEffectNode.h"
#include "UE4/FMod/Nodes/Effects/EffectNode.h"
#include "UE4/FMod/Nodes/Effects/ParameterizedEffectNode.h"
#include "UE4/FMod/Nodes/Effects/PluginEffectNode.h"
#include "UE4/FMod/Nodes/Effects/SendEffectNode.h"
#include "UE4/FMod/Nodes/Effects/SideChainEffectNode.h"
#include "UE4/FMod/Nodes/Effects/SpectralSideChainEffectNode.h"

#include "UE4/FMod/Nodes/Instruments/BaseInstrumentNode.h"
#include "UE4/FMod/Nodes/Instruments/CommandInstrumentNode.h"
#include "UE4/FMod/Nodes/Instruments/EffectInstrumentNode.h"
#include "UE4/FMod/Nodes/Instruments/EventInstrumentNode.h"
#include "UE4/FMod/Nodes/Instruments/InstrumentNode.h"
#include "UE4/FMod/Nodes/Instruments/MultiInstrumentNode.h"
#include "UE4/FMod/Nodes/Instruments/ProgrammerInstrumentNode.h"
#include "UE4/FMod/Nodes/Instruments/ScattererInstrumentNode.h"
#include "UE4/FMod/Nodes/Instruments/SilenceInstrumentNode.h"
#include "UE4/FMod/Nodes/Instruments/WaveformInstrumentNode.h"

#include "UE4/FMod/Nodes/ModulatorSubnodes/ADSRModulatorNode.h"
#include "UE4/FMod/Nodes/ModulatorSubnodes/EnvelopeModulatorNode.h"
#include "UE4/FMod/Nodes/ModulatorSubnodes/LFOModulatorNode.h"
#include "UE4/FMod/Nodes/ModulatorSubnodes/RandomModulatorNode.h"
#include "UE4/FMod/Nodes/ModulatorSubnodes/SeekModulatorNode.h"
#include "UE4/FMod/Nodes/ModulatorSubnodes/SpectralSidechainModulatorNode.h"

#include "UE4/FMod/Nodes/Transitions/BaseTransitionNode.h"
#include "UE4/FMod/Nodes/Transitions/TransitionRegionNode.h"
#include "UE4/FMod/Nodes/Transitions/TransitionTimelineNode.h"

#include "UE4/FMod/Utils/EventNodesResolver.h"
#include "UE4/FMod/Utils/JenkinsHash.h"

using namespace CUE4Parse::UE4::FMod;
using CUE4Parse::UE4::Readers::FByteArchive;
namespace Obj = CUE4Parse::UE4::FMod::Objects;
namespace Meta = CUE4Parse::UE4::FMod::Metadata;
namespace Nd = CUE4Parse::UE4::FMod::Nodes;
namespace En = CUE4Parse::UE4::FMod::Enums;
namespace Ut = CUE4Parse::UE4::FMod::Utils;

static int g_failures = 0;

// Variadic so a condition containing a comma at paren depth zero -- std::array<float, 2>, a
// std::vector<uint8_t>{...} literal -- still passes as one argument.
#define CHECK(...)                                                                  \
    do {                                                                            \
        if (!(__VA_ARGS__)) {                                                       \
            std::cerr << "FAIL: " << #__VA_ARGS__ << " (line " << __LINE__ << ")\n"; \
            ++g_failures;                                                           \
        }                                                                           \
    } while (0)

// ---------------------------------------------------------------- byte-buffer helper

// Little-endian writer, matching the format on disk.
struct Buf
{
    std::vector<uint8_t> Bytes;

    Buf& U8(uint8_t v) { Bytes.push_back(v); return *this; }
    Buf& U16(uint16_t v) { return Raw(&v, 2); }
    Buf& I16(int16_t v) { return Raw(&v, 2); }
    Buf& U32(uint32_t v) { return Raw(&v, 4); }
    Buf& I32(int32_t v) { return Raw(&v, 4); }
    Buf& U64(uint64_t v) { return Raw(&v, 8); }
    Buf& F32(float v) { return Raw(&v, 4); }
    Buf& Tag(const char* s) { for (int i = 0; i < 4; i++) U8(static_cast<uint8_t>(s[i])); return *this; }
    Buf& U24(uint32_t v) { U8(v & 0xFF); U8((v >> 8) & 0xFF); return U8((v >> 16) & 0xFF); }
    Buf& Pad(int n) { for (int i = 0; i < n; i++) U8(0); return *this; }
    Buf& Cat(const Buf& o) { Bytes.insert(Bytes.end(), o.Bytes.begin(), o.Bytes.end()); return *this; }

    Buf& Raw(const void* p, int n)
    {
        const auto* b = static_cast<const uint8_t*>(p);
        Bytes.insert(Bytes.end(), b, b + n);
        return *this;
    }

    // A four-uint32 FModGuid.
    Buf& Guid(uint32_t a, uint32_t b = 0, uint32_t c = 0, uint32_t d = 0)
    {
        return U32(a).U32(b).U32(c).U32(d);
    }

    // An empty list, for either ReadElemListImp or ReadVersionedElemListImp: ReadX16 yields 0 -> count 0,
    // and no payload size follows.
    Buf& EmptyList() { return I16(0); }
};

static FByteArchive Open(const Buf& buf) { return FByteArchive("test.bank", buf.Bytes); }

// Wraps a payload as a RIFF sub-chunk: 4-byte tag, 4-byte size, payload.
static Buf Chunk(const char* tag, const Buf& payload)
{
    Buf c;
    c.Tag(tag).U32(static_cast<uint32_t>(payload.Bytes.size())).Cat(payload);
    return c;
}

// Wraps chunks in the outer RIFF/"FEV " header. ParseHeader requires Length >= riffSize + 8.
static Buf Bank(const Buf& chunks)
{
    Buf b;
    b.Tag("RIFF").U32(static_cast<uint32_t>(chunks.Bytes.size() + 4)).Tag("FEV ").Cat(chunks);
    return b;
}

// Resets the static per-parse state so each test starts from a known version.
static void SetVersion(int fileVersion)
{
    FModReader::FormatInfo = Meta::FFormatInfo();
    FModReader::FormatInfo.FileVersion = fileVersion;
    FModReader::FormatInfo.CompatVersion = fileVersion;
    FModReader::SoundDataInfo.reset();
    FModReader::EncryptionKey.reset();
}

static Obj::FModGuid G(uint32_t a, uint32_t b = 0, uint32_t c = 0, uint32_t d = 0)
{
    Buf t; t.Guid(a, b, c, d);
    FByteArchive Ar = Open(t);
    return Obj::FModGuid(Ar);
}

// ---------------------------------------------------------------- FModGuid

static void TestFModGuid()
{
    Buf b; b.U32(0x11223344).U32(0x55667788).U32(0x99aabbcc).U32(0xddeeff00);
    FByteArchive Ar = Open(b);
    Obj::FModGuid g(Ar);

    CHECK(g.Data1 == 0x11223344);
    CHECK(g.Data2 == 0x55667788);
    CHECK(g.Data3 == 0x99aabbcc);
    CHECK(g.Data4 == 0xddeeff00);
    CHECK(Ar.Position == 16);

    CHECK(!g.IsEmpty());
    CHECK(Obj::FModGuid().IsEmpty());
    CHECK(g == g);
    CHECK(g != Obj::FModGuid());

    // FMOD reorders relative to UE: A verbatim, B rotated 16 bits, C and D byte-reversed.
    CUE4Parse::UE4::Objects::Core::Misc::FGuid fg(0x11223344, 0x55667788, 0x99aabbcc, 0xddeeff00);
    Obj::FModGuid converted(fg);
    CHECK(converted.Data1 == 0x11223344);
    CHECK(converted.Data2 == 0x77885566);
    CHECK(converted.Data3 == 0xccbbaa99);
    CHECK(converted.Data4 == 0x00ffeedd);

    // Usable as an unordered_map key (the std::hash specialization).
    std::unordered_map<Obj::FModGuid, int> m;
    m[g] = 7;
    CHECK(m.find(g) != m.end() && m[g] == 7);
    CHECK(m.find(Obj::FModGuid()) == m.end());
}

// ---------------------------------------------------------------- the global readers

static void TestGlobalReaders()
{
    // ReadX16, short form: high bit clear, value is the raw 16 bits.
    {
        Buf b; b.I16(0x1234);
        FByteArchive Ar = Open(b);
        CHECK(FModReader::ReadX16(Ar) == 0x1234u);
        CHECK(Ar.Position == 2);
    }

    // ReadX16, extended form: high bit set -> keep the low 15 bits, then a uint16 supplies bits 15+.
    {
        Buf b; b.I16(static_cast<int16_t>(0x8003)).U16(0x0002);
        FByteArchive Ar = Open(b);
        CHECK(FModReader::ReadX16(Ar) == (3u | (2u << 15)));
        CHECK(Ar.Position == 4);
    }

    // ReadString: a ReadX16 length then raw bytes -- no null terminator.
    {
        Buf b; b.I16(5).U8('h').U8('e').U8('l').U8('l').U8('o');
        FByteArchive Ar = Open(b);
        CHECK(FModReader::ReadString(Ar) == "hello");
    }
    {
        Buf b; b.I16(0);
        FByteArchive Ar = Open(b);
        CHECK(FModReader::ReadString(Ar).empty());
    }

    // ReadSimpleArray24: a count then 3 bytes per entry, widened little-endian.
    {
        Buf b; b.I16(3).U24(0x000001).U24(0xABCDEF).U24(0xFFFFFF);
        FByteArchive Ar = Open(b);
        std::vector<Obj::FUInt24> arr = FModReader::ReadSimpleArray24(Ar);
        CHECK(arr.size() == 3);
        CHECK(arr[0].Value == 0x000001u);
        CHECK(arr[1].Value == 0xABCDEFu);
        CHECK(arr[2].Value == static_cast<uint32_t>(Obj::FRadixTreePacked::Sentinel24));
        CHECK(Ar.Position == 2 + 9);
    }

    // ReadElemListImp: count is ReadX16 >> 1, and ONE uint16 payload size precedes the whole list.
    {
        Buf b; b.I16(4).U16(16).Guid(1).Guid(2);
        FByteArchive Ar = Open(b);
        auto list = FModReader::ReadElemListImp<Obj::FModGuid>(Ar);
        CHECK(list.size() == 2);
        CHECK(list[0].Data1 == 1);
        CHECK(list[1].Data1 == 2);
        CHECK(Ar.Position == 2 + 2 + 32);
    }

    // ReadVersionedElemListImp: same count, but a uint16 payload size precedes EACH element.
    {
        Buf b; b.I16(4).U16(16).Guid(1).U16(16).Guid(2);
        FByteArchive Ar = Open(b);
        auto list = FModReader::ReadVersionedElemListImp<Obj::FModGuid>(Ar);
        CHECK(list.size() == 2);
        CHECK(list[0].Data1 == 1);
        CHECK(list[1].Data1 == 2);
        CHECK(Ar.Position == 2 + (2 + 16) * 2);
    }

    // Both list readers stop before touching a payload size when the count is zero.
    {
        Buf b; b.EmptyList().U32(0xDEADBEEF);
        FByteArchive Ar = Open(b);
        CHECK(FModReader::ReadElemListImp<Obj::FModGuid>(Ar).empty());
        CHECK(Ar.Read<uint32_t>() == 0xDEADBEEFu);
    }

    // The getter overloads, used where the element is a bare primitive rather than a struct.
    {
        Buf b; b.I16(4).U16(4).U32(11).U32(22);
        FByteArchive Ar = Open(b);
        auto list = FModReader::ReadElemListImp(Ar, [](CUE4Parse::UE4::Readers::FArchive& a) { return a.Read<uint32_t>(); });
        CHECK(list.size() == 2 && list[0] == 11u && list[1] == 22u);
    }
    {
        Buf b; b.I16(2).U16(3).I16(2).U8('h').U8('i');
        FByteArchive Ar = Open(b);
        auto list = FModReader::ReadVersionedElemListImp(Ar, [](CUE4Parse::UE4::Readers::FArchive& a) { return FModReader::ReadString(a); });
        CHECK(list.size() == 1 && list[0] == "hi");
    }
}

// ---------------------------------------------------------------- FEvaluator

static void TestEvaluator()
{
    SetVersion(0x83);

    // The low byte of the raw uint is the type; the upper bytes are flags and must be masked off.
    {
        Buf b; b.U32(0xFF00'0000u | static_cast<uint32_t>(En::EEvaluatorType::Type10)).U32(1234);
        FByteArchive Ar = Open(b);
        Obj::FEvaluator ev(Ar);
        CHECK(ev.Type == En::EEvaluatorType::Type10);
        CHECK(std::holds_alternative<uint32_t>(ev.Data));
        CHECK(std::get<uint32_t>(ev.Data) == 1234u);
    }
    {
        Buf b; b.U32(static_cast<uint32_t>(En::EEvaluatorType::Type11)).Guid(9, 8, 7, 6);
        FByteArchive Ar = Open(b);
        Obj::FEvaluator ev(Ar);
        CHECK(std::holds_alternative<Obj::FModGuid>(ev.Data));
        CHECK(std::get<Obj::FModGuid>(ev.Data) == G(9, 8, 7, 6));
    }
    {
        Buf b; b.U32(static_cast<uint32_t>(En::EEvaluatorType::Type20)).F32(1.5f).F32(-2.5f);
        FByteArchive Ar = Open(b);
        Obj::FEvaluator ev(Ar);
        CHECK(std::holds_alternative<std::array<float, 2>>(ev.Data));
        CHECK(std::get<std::array<float, 2>>(ev.Data)[0] == 1.5f);
        CHECK(std::get<std::array<float, 2>>(ev.Data)[1] == -2.5f);
    }
    {
        Buf b; b.U32(static_cast<uint32_t>(En::EEvaluatorType::Basic0));
        FByteArchive Ar = Open(b);
        Obj::FEvaluator ev(Ar);
        CHECK(std::holds_alternative<std::monostate>(ev.Data));
        CHECK(Ar.Position == 4);
    }

    // ReadEvaluatorList is bounded by a leading total byte size, not a count.
    {
        Buf payload;
        payload.U32(static_cast<uint32_t>(En::EEvaluatorType::Basic1));
        payload.U32(static_cast<uint32_t>(En::EEvaluatorType::Type10)).U32(77);
        Buf b; b.I32(static_cast<int32_t>(payload.Bytes.size())).Cat(payload).U32(0xDEADBEEF);
        FByteArchive Ar = Open(b);
        auto list = Obj::FEvaluator::ReadEvaluatorList(Ar);
        CHECK(list.size() == 2);
        CHECK(list[0].Type == En::EEvaluatorType::Basic1);
        CHECK(std::get<uint32_t>(list[1].Data) == 77u);
        CHECK(Ar.Read<uint32_t>() == 0xDEADBEEFu); // landed exactly on the declared end
    }
    {
        Buf b; b.I32(0);
        FByteArchive Ar = Open(b);
        CHECK(Obj::FEvaluator::ReadEvaluatorList(Ar).empty());
    }
}

// ---------------------------------------------------------------- Metadata

static void TestMetadata()
{
    // FFormatInfo, and the fact FModReader::Version() reads off the static instance.
    {
        Buf b; b.I32(0x8A).I32(0x60);
        FByteArchive Ar = Open(b);
        Meta::FFormatInfo fmt(Ar);
        CHECK(fmt.FileVersion == 0x8A);
        CHECK(fmt.CompatVersion == 0x60);
        FModReader::FormatInfo = fmt;
        CHECK(FModReader::Version() == 0x8A);
    }

    // FBankInfo's version gates: Hash at 0x37, TopLevelEventCount at 0x41, ExportFlags at 0x4D.
    {
        SetVersion(0x36);
        Buf b; b.Guid(5).U64(0xAAAA); // the u64 must NOT be consumed at 0x36
        FByteArchive Ar = Open(b);
        Meta::FBankInfo info(Ar);
        CHECK(info.BaseGuid == G(5));
        CHECK(info.Hash == 0);
        CHECK(info.TopLevelEventCount == 0);
        CHECK(Ar.Position == 16);
    }
    {
        SetVersion(0x4D);
        Buf b; b.Guid(5).U64(0x1122334455667788ull).I32(3).I32(9);
        FByteArchive Ar = Open(b);
        Meta::FBankInfo info(Ar);
        CHECK(info.Hash == 0x1122334455667788ull);
        CHECK(info.TopLevelEventCount == 3);
        CHECK(info.ExportFlags == 9);
        CHECK(info.FileVersion == static_cast<En::EFModVersion>(0x4D));
        CHECK(Ar.Position == 16 + 8 + 4 + 4);
    }

    // SoundTable: header, keys, 24-bit indices, and (>= 0x7c) flags. Find is a binary search over the
    // sorted key array, returning the parallel index entry.
    {
        SetVersion(0x7c);
        Buf b;
        b.U32(1).I32(2);
        b.I16(3).U64(100).U64(200).U64(300);
        b.I16(3).U24(10).U24(20).U24(30);
        b.U32(0x55);
        FByteArchive Ar = Open(b);
        Meta::SoundTable table(Ar);
        CHECK(table.HeaderFlag == 1u);
        CHECK(table.SoundbankIndex == 2);
        CHECK(table.Keys.size() == 3 && table.Indices.size() == 3);
        CHECK(table.Flags == 0x55u);
        CHECK(table.Find(100) == 10);
        CHECK(table.Find(200) == 20);
        CHECK(table.Find(300) == 30);
        CHECK(table.Find(150) == -1);
        CHECK(table.Find(400) == -1);
    }
    {
        // An empty table must return -1 rather than indexing with high = -1.
        SetVersion(0x7b);
        Buf b; b.U32(0).I32(0).I16(0).I16(0);
        FByteArchive Ar = Open(b);
        Meta::SoundTable table(Ar);
        CHECK(table.Keys.empty());
        CHECK(table.Find(1) == -1);
    }

    // StringTable only builds a radix tree for the 24-bit variant.
    {
        SetVersion(0x83);
        Buf b; b.U32(static_cast<uint32_t>(En::EStringTableType::StringTable_RadixTree_32Bit));
        FByteArchive Ar = Open(b);
        Meta::StringTable st(Ar);
        CHECK(st.Type == En::EStringTableType::StringTable_RadixTree_32Bit);
        CHECK(!st.RadixTree.has_value());
    }
}

// ---------------------------------------------------------------- FRadixTreePacked

static void TestRadixTree()
{
    SetVersion(0x83);

    // A two-level tree: node 0 = "event:/", node 1 = "Music" whose parent is node 0. One guid, whose leaf
    // is node 1, so the path is assembled root-first: "event:/" + "Music".
    const char* blob = "event:/\0Music\0";
    const int blobLen = 14;
    const int offRoot = 0;
    const int offMusic = 8;

    Buf payload;
    payload.U32(static_cast<uint32_t>(En::EStringTableType::StringTable_RadixTree_24Bit));

    // Nodes: ReadElemListImp<FPackedNode>, 8 bytes each.
    payload.I16(4).U16(8);
    payload.U32(static_cast<uint32_t>(offRoot)).U32(0);
    payload.U32(static_cast<uint32_t>(offMusic)).U32(0);

    // Guids: one entry.
    payload.I16(2).U16(16).Guid(0xC0FFEE);

    // StringBlob: a ReadX16 count then raw bytes.
    payload.I16(blobLen).Raw(blob, blobLen);

    // LeafIndices (one per guid) then ParentIndices (one per node).
    payload.I16(1).U24(1);
    payload.I16(2).U24(Obj::FRadixTreePacked::Sentinel24).U24(0);

    FByteArchive Ar = Open(payload);
    Meta::StringTable st(Ar);
    CHECK(st.RadixTree.has_value());
    if (!st.RadixTree.has_value()) return;

    const Obj::FRadixTreePacked& tree = *st.RadixTree;
    CHECK(tree.Nodes.size() == 2);
    CHECK(tree.Guids.size() == 1);
    CHECK(tree.StringBlob.size() == static_cast<size_t>(blobLen));
    CHECK(tree.LeafIndices.size() == 1);
    CHECK(tree.ParentIndices.size() == 2);
    CHECK(tree.Nodes[0].HasString());
    CHECK(tree.Nodes[0].StringOffset() == offRoot);

    std::string path;
    CHECK(tree.TryGetString(G(0xC0FFEE), path));
    CHECK(path == "event:/Music");

    // An unknown guid clears the out-param and reports failure.
    path = "stale";
    CHECK(!tree.TryGetString(G(1), path));
    CHECK(path.empty());

    // A sentinel leaf means "no path", but is still a successful lookup.
    CHECK(tree.TryGetStringByIndex(-1, path) == false);
}

// ---------------------------------------------------------------- version-gated node reads

static void TestVersionGates()
{
    // WaveformResourceNode gains LoadingMode at 0x46. Below that, the four bytes belong to whatever comes
    // next -- so the archive position is the assertion that matters.
    {
        SetVersion(0x45);
        Buf b; b.Guid(1).U16(12).I32(3).I32(4).U32(0xDEADBEEF);
        FByteArchive Ar = Open(b);
        Nd::WaveformResourceNode wav(Ar);
        CHECK(wav.BaseGuid == G(1));
        CHECK(wav.SoundBankIndex == 3);
        CHECK(wav.SubsoundIndex == 4);
        CHECK(Ar.Position == 16 + 2 + 8);
        CHECK(Ar.Read<uint32_t>() == 0xDEADBEEFu);
    }
    {
        SetVersion(0x46);
        Buf b; b.Guid(1).U16(16).I32(3).I32(4).U32(1);
        FByteArchive Ar = Open(b);
        Nd::WaveformResourceNode wav(Ar);
        CHECK(wav.LoadingMode == static_cast<En::EWaveformLoadingMode>(1));
        CHECK(Ar.Position == 16 + 2 + 12);
    }

    // WaveformInstrumentNode drops LegacyLoadingMode at 0x46 (a rare shrink rather than a growth).
    {
        SetVersion(0x45);
        Buf b; b.Guid(1).U32(2).Guid(7);
        FByteArchive Ar = Open(b);
        Nd::Instruments::WaveformInstrumentNode inst(Ar);
        CHECK(inst.BaseGuid == G(1));
        CHECK(inst.LegacyLoadingMode == static_cast<En::EWaveformLoadingMode>(2));
        CHECK(inst.WaveformResourceGuid == G(7));
        CHECK(Ar.Position == 36);
    }
    {
        SetVersion(0x46);
        Buf b; b.Guid(1).Guid(7);
        FByteArchive Ar = Open(b);
        Nd::Instruments::WaveformInstrumentNode inst(Ar);
        CHECK(inst.WaveformResourceGuid == G(7));
        CHECK(Ar.Position == 32);
    }

    // TimelineNode swaps a legacy uint list for versioned sustain points at 0x84, and drops LegacyGuid
    // at 0x6d.
    {
        SetVersion(0x6c);
        Buf b;
        b.Guid(1).Guid(2);      // BaseGuid, LegacyGuid
        b.I16(2).U16(24).Guid(0x101).U32(5).U32(6); // one TriggerBox
        b.EmptyList();          // TimeLockedTriggerBoxes
        b.I16(4).U16(4).U32(70).U32(80); // LegacyUIntArray (< 0x84)
        b.EmptyList();          // TimelineNamedMarkers
        b.EmptyList();          // TimelineTempoMarkers
        FByteArchive Ar = Open(b);
        Nd::TimelineNode tml(Ar);
        CHECK(tml.BaseGuid == G(1));
        CHECK(tml.LegacyGuid == G(2));
        CHECK(tml.TriggerBoxes.size() == 1);
        CHECK(tml.TriggerBoxes[0].Guid == G(0x101));
        CHECK(tml.TriggerBoxes[0].StartTime == 5u);
        CHECK(tml.TriggerBoxes[0].Length == 6u);
        CHECK(tml.LegacyUIntArray.size() == 2 && tml.LegacyUIntArray[1] == 80u);
        CHECK(tml.SustainPoints.empty());
        CHECK(Ar.Position == static_cast<int64_t>(b.Bytes.size()));
    }

    // ParameterLayoutNode: at 0x82+ it reads an instrument guid list and returns early; below that it
    // reads trigger boxes instead.
    {
        SetVersion(0x82);
        Buf b; b.Guid(1).Guid(2).I16(2).U16(16).Guid(0x201).U32(0x77);
        FByteArchive Ar = Open(b);
        Nd::ParameterLayoutNode pl(Ar);
        CHECK(pl.BaseGuid == G(1));
        CHECK(pl.ParameterGuid == G(2));
        CHECK(pl.Instruments.size() == 1 && pl.Instruments[0] == G(0x201));
        CHECK(pl.TriggerBoxes.empty());
        CHECK(pl.Flags == 0x77u);
        CHECK(Ar.Position == static_cast<int64_t>(b.Bytes.size()));
    }
    {
        SetVersion(0x6a);
        // A third guid here: below 0x6d the node also carries LegacyGuid.
        Buf b; b.Guid(1).Guid(2).Guid(3).I16(2).U16(25).Guid(0x201).F32(0.25f).F32(0.75f).U8(1).U32(0x88);
        FByteArchive Ar = Open(b);
        Nd::ParameterLayoutNode pl(Ar);
        CHECK(pl.LegacyGuid == G(3));
        CHECK(pl.Instruments.empty());
        CHECK(pl.TriggerBoxes.size() == 1);
        CHECK(pl.TriggerBoxes[0].InstrumentGuid == G(0x201));
        CHECK(pl.TriggerBoxes[0].Start == 0.25f);
        CHECK(pl.TriggerBoxes[0].End == 0.75f);
        CHECK(pl.TriggerBoxes[0].IncludeEnd == true); // ONE byte, not four
        CHECK(pl.Flags == 0x88u);
        CHECK(Ar.Position == static_cast<int64_t>(b.Bytes.size()));
    }

    // TransitionRegionNode's three-way split on how conditions are stored: legacy struct (< 0x43), a
    // FirstOrDefault over a list (0x43..0x81), or an evaluator list (0x82+).
    {
        SetVersion(0x83);
        Buf b;
        b.Guid(1).Guid(2).U32(10).U32(20);
        b.I32(0);                                                          // empty evaluator list
        b.U32(static_cast<uint32_t>(En::EQuantizationUnit::None)).I32(4);   // FQuantization
        b.F32(0.5f).U32(0x99);
        FByteArchive Ar = Open(b);
        Nd::Transitions::TransitionRegionNode tr(Ar);
        CHECK(tr.BaseGuid == G(1));
        CHECK(tr.DestinationGuid == G(2));
        CHECK(tr.Start == 10u && tr.End == 20u);
        CHECK(tr.Evaluators.empty());
        CHECK(!tr.LegacyParameterConditions.has_value());
        CHECK(tr.Quantization.Unit == En::EQuantizationUnit::None);
        CHECK(tr.Quantization.Multiplier == 4);
        CHECK(tr.TransitionChancePercent == 0.5f);
        CHECK(tr.Flags == 0x99u);
        CHECK(Ar.Position == static_cast<int64_t>(b.Bytes.size()));
    }

    // FQuantization skips the multiplier entirely for units past EighthNote.
    {
        Buf b; b.U32(static_cast<uint32_t>(En::EQuantizationUnit::Max)).U32(0xDEADBEEF);
        FByteArchive Ar = Open(b);
        Obj::FQuantization q(Ar);
        CHECK(q.Multiplier == 0);
        CHECK(Ar.Position == 4);
        CHECK(Ar.Read<uint32_t>() == 0xDEADBEEFu);
    }
}

// ---------------------------------------------------------------- JenkinsHash

static void TestJenkinsHash()
{
    // The empty input takes the `case 0` early return, so both halves are the seed: 0xdeadbeef + 0 + pc.
    CHECK(Ut::JenkinsHash::Hash64("") == 0xdeadbeefdeadbeefull);

    // Deterministic, and sensitive to every byte -- including the >12-byte block loop.
    const std::string a = "event:/Music/Combat";
    const std::string b = "event:/Music/Combau";
    CHECK(a.size() > 12);
    CHECK(Ut::JenkinsHash::Hash64(a) == Ut::JenkinsHash::Hash64(a));
    CHECK(Ut::JenkinsHash::Hash64(a) != Ut::JenkinsHash::Hash64(b));
    CHECK(Ut::JenkinsHash::Hash64(a) != 0);

    // The tail switch covers lengths 1..12; each must be distinct from its prefix.
    uint64_t prev = Ut::JenkinsHash::Hash64("");
    for (size_t n = 1; n <= 12; n++)
    {
        uint64_t h = Ut::JenkinsHash::Hash64(a.substr(0, n));
        CHECK(h != prev);
        prev = h;
    }

    // The initval parameters feed the seed, so they change the result.
    CHECK(Ut::JenkinsHash::Hash64(a, 1, 0) != Ut::JenkinsHash::Hash64(a, 0, 0));
    CHECK(Ut::JenkinsHash::Hash64(a, 0, 1) != Ut::JenkinsHash::Hash64(a, 0, 0));
}

// ---------------------------------------------------------------- Fsb5Decryption

static void TestFsb5Decryption()
{
    CHECK(Fsb5Decryption::IsFSB5Header(std::vector<uint8_t>{'F', 'S', 'B', '5'}));
    CHECK(!Fsb5Decryption::IsFSB5Header(std::vector<uint8_t>{'F', 'S', 'B', '4'}));
    CHECK(!Fsb5Decryption::IsFSB5Header(std::vector<uint8_t>{'F', 'S', 'B'})); // too short
    CHECK(Fsb5Decryption::IsFSB5Header(std::vector<uint8_t>{0, 0, 'F', 'S', 'B', '5'}, 2));

    // Decryption is bit-reverse-then-XOR. To build a matching ciphertext, XOR first and bit-reverse after:
    // reverse(reverse(x)) == x, so the transform is its own inverse in that order.
    auto reverseBits = [](uint8_t v)
    {
        uint8_t r = 0;
        for (int i = 0; i < 8; i++) if (v & (1u << i)) r |= static_cast<uint8_t>(1u << (7 - i));
        return r;
    };

    const std::vector<uint8_t> key{ 0x5A, 0x13, 0xF0 };
    std::vector<uint8_t> plain{ 'F', 'S', 'B', '5', 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00 };

    std::vector<uint8_t> cipher(plain.size());
    for (size_t i = 0; i < plain.size(); i++)
        cipher[i] = reverseBits(static_cast<uint8_t>(plain[i] ^ key[i % key.size()]));

    CHECK(!Fsb5Decryption::IsFSB5Header(cipher));
    Fsb5Decryption::DecryptInPlace(cipher, key);
    CHECK(cipher == plain);
    CHECK(Fsb5Decryption::IsFSB5Header(cipher));

    // An empty key is an error, not a no-op.
    bool threw = false;
    try { std::vector<uint8_t> d{ 1, 2, 3 }; Fsb5Decryption::DecryptInPlace(d, {}); }
    catch (const std::runtime_error&) { threw = true; }
    CHECK(threw);
}

// ---------------------------------------------------------------- the chunk walker

static void TestParseHeader()
{
    SetVersion(0x83);

    auto parseThrows = [](const Buf& b)
    {
        FByteArchive Ar = Open(b);
        try { FModReader reader(Ar, "test.bank"); }
        catch (const std::runtime_error&) { return true; }
        catch (...) { return false; }
        return false;
    };

    CHECK(parseThrows(Buf().Tag("RIFF").U32(4)));                    // under 12 bytes
    CHECK(parseThrows(Buf().Tag("RIFX").U32(4).Tag("FEV ")));         // not RIFF
    CHECK(parseThrows(Buf().Tag("RIFF").U32(4).Tag("WAVE")));         // not an FMOD bank
    CHECK(parseThrows(Buf().Tag("RIFF").U32(999).Tag("FEV ")));       // truncated vs. the declared size

    // A bank with only the header parses to an empty reader.
    {
        Buf b = Bank(Buf());
        FByteArchive Ar = Open(b);
        FModReader reader(Ar, "empty.bank");
        CHECK(reader.BankName == "empty.bank");
        CHECK(reader.EventNodes.empty());
        CHECK(reader.GetBankGuid().IsEmpty()); // no BNKI chunk
    }

    // A zero-size chunk is skipped, and an unknown tag is stepped over by the corrective seek rather than
    // derailing the walk.
    {
        Buf chunks;
        chunks.Cat(Chunk("FMT ", Buf().I32(0x83).I32(0x83)));
        chunks.Cat(Chunk("ZZZZ", Buf().U64(0).U64(0)));
        chunks.Tag("NADA").U32(0);
        chunks.Cat(Chunk("BNKI", Buf().Guid(0xB4A4).U64(1).I32(2).I32(3)));
        Buf b = Bank(chunks);
        FByteArchive Ar = Open(b);
        FModReader reader(Ar, "skip.bank");
        CHECK(FModReader::Version() == 0x83);
        CHECK(reader.BankInfo != nullptr);
        CHECK(reader.GetBankGuid() == G(0xB4A4));
    }

    // A LIST chunk recurses into its sub-chunks (after a 4-byte list id).
    {
        Buf inner;
        inner.Cat(Chunk("WAV ", Buf().Guid(0x11).U16(16).I32(0).I32(0).U32(0)));
        inner.Cat(Chunk("WAV ", Buf().Guid(0x22).U16(16).I32(0).I32(1).U32(0)));

        Buf chunks;
        chunks.Cat(Chunk("FMT ", Buf().I32(0x83).I32(0x83)));
        chunks.Cat(Chunk("LIST", Buf().Tag("WAVS").Cat(inner)));
        Buf b = Bank(chunks);
        FByteArchive Ar = Open(b);
        FModReader reader(Ar, "list.bank");
        CHECK(reader.WavEntries.size() == 2);
        CHECK(reader.WavEntries.count(G(0x11)) == 1);
        CHECK(reader.WavEntries.count(G(0x22)) == 1);
        if (reader.WavEntries.count(G(0x22)))
            CHECK(reader.WavEntries[G(0x22)]->SubsoundIndex == 1);
    }
}

// ---------------------------------------------------------------- a whole synthetic bank + the resolver

// Guids used by the bank below.
static const uint32_t GID_EVENT = 0xE0;
static const uint32_t GID_TML   = 0x70;
static const uint32_t GID_PML   = 0x60;
static const uint32_t GID_TRAN  = 0x50;
static const uint32_t GID_INST1 = 0x11, GID_INST2 = 0x12, GID_INST3 = 0x13;
static const uint32_t GID_WAV1  = 0x21, GID_WAV2  = 0x22, GID_WAV3  = 0x23;

// Builds a v0x83 bank wiring an event to three waveform instruments by three different routes:
// a timeline trigger box, the event's own EventTriggeredInstruments list, and a parameter layout.
static Buf BuildBank()
{
    Buf chunks;
    chunks.Cat(Chunk("FMT ", Buf().I32(0x83).I32(0x83)));
    chunks.Cat(Chunk("BNKI", Buf().Guid(0xB4A4).U64(0xABCD).I32(1).I32(0)));

    // EVTB -- EventNode at 0x83.
    {
        Buf p;
        p.Guid(GID_EVENT).Guid(0).Guid(GID_TML).Guid(0).Guid(0);
        p.I32(16).I32(1).U8(1).I32(2);                  // MaxPolyphony, Priority, PolyphonyLimitBehavior(1 byte!), SchedulingMode
        p.I16(2).U16(16).Guid(GID_PML);                 // ParameterLayouts
        p.EmptyList();                                  // UserPropertyFloatList
        p.EmptyList();                                  // UserPropertyStringList
        p.F32(1.0f);                                    // DopplerScale     (>= 0x30)
        p.I32(1);                                       // PolyphonyLimitBehavior again, now 4 bytes (>= 0x34)
        p.F32(0.25f);                                   // TriggerCooldown  (>= 0x4e)
        p.U32(0x8000);                                  // Flags            (>= 0x61)
        p.EmptyList();                                  // NonMasterTracks  (>= 0x6b)
        p.EmptyList();                                  // ParameterIds     (>= 0x76)
        p.I16(2).U16(16).Guid(GID_INST2);               // EventTriggeredInstruments (>= 0x83)
        chunks.Cat(Chunk("EVTB", p));
    }

    // TLNB -- TimelineNode at 0x83, one trigger box aimed at instrument 1.
    {
        Buf p;
        p.Guid(GID_TML);
        p.I16(2).U16(24).Guid(GID_INST1).U32(0).U32(1000); // TriggerBoxes
        p.EmptyList();                                     // TimeLockedTriggerBoxes
        p.EmptyList();                                     // LegacyUIntArray (< 0x84)
        p.EmptyList();                                     // TimelineNamedMarkers
        p.EmptyList();                                     // TimelineTempoMarkers
        chunks.Cat(Chunk("TLNB", p));
    }

    // PMLB -- ParameterLayoutNode at 0x83, listing instrument 3.
    chunks.Cat(Chunk("PMLB", Buf().Guid(GID_PML).Guid(0).I16(2).U16(16).Guid(GID_INST3).U32(0)));

    // TRNB -- a transition region whose destination is the timeline, so the resolver's destinationLookup
    // finds it during the walk. No TRTL body follows, so TransitionBody stays null.
    {
        Buf p;
        p.Guid(GID_TRAN).Guid(GID_TML).U32(0).U32(100);
        p.I32(0);                                                        // evaluators (>= 0x82)
        p.U32(static_cast<uint32_t>(En::EQuantizationUnit::None)).I32(1); // FQuantization
        p.F32(1.0f).U32(0);
        chunks.Cat(Chunk("TRNB", p));
    }

    // WAIB x3 -- waveform instruments, each pointing at a WAV resource.
    chunks.Cat(Chunk("WAIB", Buf().Guid(GID_INST1).Guid(GID_WAV1)));
    chunks.Cat(Chunk("WAIB", Buf().Guid(GID_INST2).Guid(GID_WAV2)));
    chunks.Cat(Chunk("WAIB", Buf().Guid(GID_INST3).Guid(GID_WAV3)));

    // WAV x3 -- subsounds 0, 1 and 2 of soundbank 0.
    chunks.Cat(Chunk("WAV ", Buf().Guid(GID_WAV1).U16(16).I32(0).I32(0).U32(0)));
    chunks.Cat(Chunk("WAV ", Buf().Guid(GID_WAV2).U16(16).I32(0).I32(1).U32(0)));
    chunks.Cat(Chunk("WAV ", Buf().Guid(GID_WAV3).U16(16).I32(0).I32(2).U32(0)));

    return Bank(chunks);
}

static void TestWholeBank()
{
    SetVersion(0x83);
    Buf b = BuildBank();
    FByteArchive Ar = Open(b);
    FModReader reader(Ar, "synthetic.bank");

    CHECK(FModReader::Version() == 0x83);
    CHECK(reader.GetBankGuid() == G(0xB4A4));
    CHECK(reader.BankInfo != nullptr && reader.BankInfo->TopLevelEventCount == 1);

    CHECK(reader.EventNodes.size() == 1);
    CHECK(reader.TimelineNodes.size() == 1);
    CHECK(reader.ParameterLayoutNodes.size() == 1);
    CHECK(reader.TransitionNodes.size() == 1);
    CHECK(reader.InstrumentNodes.size() == 3);
    CHECK(reader.WavEntries.size() == 3);

    // Every chunk landed exactly where the walker expected: if any one field's width were wrong the
    // corrective seek would have papered over it, but the parsed values would be garbage.
    auto ev = reader.EventNodes.find(G(GID_EVENT));
    CHECK(ev != reader.EventNodes.end());
    if (ev != reader.EventNodes.end())
    {
        const Nd::EventNode& e = *ev->second;
        CHECK(e.TimelineGuid == G(GID_TML));
        CHECK(e.MaximumPolyphony == 16);
        CHECK(e.Priority == 1);
        CHECK(e.SchedulingMode == 2);
        CHECK(e.DopplerScale == 1.0f);
        CHECK(e.TriggerCooldown == 0.25f);
        CHECK(e.Flags == 0x8000u);
        CHECK(e.ParameterLayouts.size() == 1 && e.ParameterLayouts[0] == G(GID_PML));
        CHECK(e.EventTriggeredInstruments.size() == 1 && e.EventTriggeredInstruments[0] == G(GID_INST2));
        CHECK(e.MinimumDistance == 0.0f); // < 0x89, so never read
    }

    auto tml = reader.TimelineNodes.find(G(GID_TML));
    CHECK(tml != reader.TimelineNodes.end());
    if (tml != reader.TimelineNodes.end())
    {
        CHECK(tml->second->TriggerBoxes.size() == 1);
        CHECK(tml->second->TriggerBoxes[0].Guid == G(GID_INST1));
        CHECK(tml->second->TriggerBoxes[0].Length == 1000u);
    }

    // The transition is stored polymorphically; the walker's dynamic_cast is what the resolver relies on.
    auto tr = reader.TransitionNodes.find(G(GID_TRAN));
    CHECK(tr != reader.TransitionNodes.end());
    if (tr != reader.TransitionNodes.end())
    {
        auto* region = dynamic_cast<Nd::Transitions::TransitionRegionNode*>(tr->second.get());
        CHECK(region != nullptr);
        if (region) CHECK(region->DestinationGuid == G(GID_TML));
        CHECK(tr->second->TransitionBody == nullptr); // no TRTL chunk followed
    }

    // Instruments are polymorphic too, and none got an INST body chunk.
    auto inst = reader.InstrumentNodes.find(G(GID_INST1));
    CHECK(inst != reader.InstrumentNodes.end());
    if (inst != reader.InstrumentNodes.end())
    {
        auto* wavInst = dynamic_cast<Nd::Instruments::WaveformInstrumentNode*>(inst->second.get());
        CHECK(wavInst != nullptr);
        if (wavInst) CHECK(wavInst->WaveformResourceGuid == G(GID_WAV1));
        CHECK(inst->second->InstrumentBody == nullptr);
    }

    // ---- EventNodesResolver ----
    // The FSB5 decode is out of scope, so the bank stub only carries a sample count; that count is what
    // bounds-checks a WaveformResourceNode's subsound index (C# checks against bank.Samples.Count).
    FModSoundBank bank;
    bank.SampleCount = 3;
    reader.SoundBankData.push_back(bank);

    bool allResolved = false;
    auto resolved = Ut::EventNodesResolver::TryResolveAudioEvents(reader, allResolved);
    CHECK(resolved.size() == 1);
    CHECK(allResolved);

    auto it = resolved.find(G(GID_EVENT));
    CHECK(it != resolved.end());
    if (it != resolved.end())
    {
        const std::vector<Ut::FWaveformRef>& refs = it->second;
        CHECK(refs.size() == 3);

        bool has0 = false, has1 = false, has2 = false;
        for (const auto& r : refs)
        {
            CHECK(r.SoundBankIndex == 0);
            if (r.SubsoundIndex == 0) has0 = true;
            if (r.SubsoundIndex == 1) has1 = true;
            if (r.SubsoundIndex == 2) has2 = true;
        }
        CHECK(has0); // reached via the timeline trigger box
        CHECK(has1); // reached via EventTriggeredInstruments
        CHECK(has2); // reached via the parameter layout
    }

    // Shrinking the sample count must drop the out-of-range subsound and report an incomplete resolve.
    reader.SoundBankData[0].SampleCount = 2;
    allResolved = true;
    resolved = Ut::EventNodesResolver::TryResolveAudioEvents(reader, allResolved);
    CHECK(!allResolved);
    it = resolved.find(G(GID_EVENT));
    if (it != resolved.end())
    {
        CHECK(it->second.size() == 2);
        for (const auto& r : it->second)
            CHECK(r.SubsoundIndex < 2);
    }

    // With no soundbank at all nothing resolves, but the walk itself must still complete.
    reader.SoundBankData.clear();
    resolved = Ut::EventNodesResolver::TryResolveAudioEvents(reader, allResolved);
    CHECK(resolved.size() == 1);
    CHECK(resolved[G(GID_EVENT)].empty());
    CHECK(!allResolved);
}

// ---------------------------------------------------------------- Merge

static void TestMerge()
{
    SetVersion(0x83);

    Buf b1 = BuildBank();
    FByteArchive Ar1 = Open(b1);
    FModReader dst(Ar1, "a.bank");

    // A second bank with a different event/wav set.
    Buf chunks;
    chunks.Cat(Chunk("FMT ", Buf().I32(0x83).I32(0x83)));
    chunks.Cat(Chunk("WAV ", Buf().Guid(0x900).U16(16).I32(1).I32(0).U32(0)));
    Buf b2 = Bank(chunks);
    FByteArchive Ar2 = Open(b2);
    FModReader src(Ar2, "b.bank");
    src.SoundBankData.push_back(FModSoundBank{});
    src.ControllerOwnerNodes.push_back(G(0xC0));

    const size_t wavsBefore = dst.WavEntries.size();
    dst.Merge(src);

    CHECK(dst.WavEntries.size() == wavsBefore + 1);
    CHECK(dst.WavEntries.count(G(0x900)) == 1);
    CHECK(dst.SoundBankData.size() == 1);
    CHECK(dst.ControllerOwnerNodes.size() == 1 && dst.ControllerOwnerNodes[0] == G(0xC0));

    // Merge moves the unique_ptrs out of the source map (the entries remain, now null).
    CHECK(src.WavEntries[G(0x900)] == nullptr);
}

// ----------------------------------------------------------------

int main()
{
    TestFModGuid();
    TestGlobalReaders();
    TestEvaluator();
    TestMetadata();
    TestRadixTree();
    TestVersionGates();
    TestJenkinsHash();
    TestFsb5Decryption();
    TestParseHeader();
    TestWholeBank();
    TestMerge();

    if (g_failures != 0)
    {
        std::cerr << g_failures << " check(s) failed\n";
        return 1;
    }

    std::cout << "test_fmod_bank: all checks passed\n";
    return 0;
}
