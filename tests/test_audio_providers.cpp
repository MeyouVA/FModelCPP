// Tests the two audio provider layers -- CUE4Parse/UE4/Wwise/WwiseProvider.cs and
// CUE4Parse/UE4/FMod/FModProvider.cs -- end to end, over real containers mounted through a real pak.
//
// Both providers are the join between "one container parsed" and "which sounds does this export play?", so
// the interesting behaviour is not in any single function: it is that a bank authored on the wire gets
// bulk-loaded, indexed into the flat hierarchy table, and then walked from an event id down to a .wem whose
// bytes come back through a deferred read of the pak entry. That whole path is what the Wwise test below
// exercises; nothing is stubbed except the game itself.
//
// The Wwise bank is a genuine v145 .bnk: BKHD, DIDX/DATA carrying one 8-byte media blob, and a HIRC of
// three entries wiring Event -> EventAction(Play) -> FxCustom -> media id 42. The byte layouts come from
// the readers under UE4/Wwise/Objects and match the fixtures in test_wwise.cpp.
//
// The FMOD bank is the same synthetic v0x83 "FEV " bank as test_fmod_bank.cpp (event -> timeline /
// parameter layout / triggered instruments -> three waveforms), here mounted in a pak under Content/FMOD so
// FModProvider's own discovery, grouping and merging runs over it.
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "FileProvider/Vfs/AbstractVfsFileProvider.h"
#include "UE4/Assets/Exports/Wwise/UAkAudioEvent.h"
#include "UE4/FMod/FModProvider.h"
#include "UE4/FMod/FModReader.h"
#include "UE4/FMod/FModSoundBank.h"
#include "UE4/FMod/Metadata/FFormatInfo.h"
#include "UE4/FMod/Metadata/SoundDataInfo.h" // FModReader::SoundDataInfo is a unique_ptr to it; reset() needs the definition
#include "UE4/Objects/Core/Misc/FGuid.h"
#include "UE4/Pak/Objects/FPakInfo.h"
#include "UE4/Readers/FByteArchive.h"
#include "UE4/Wwise/Enums/EAKBKHircType.h"
#include "UE4/Wwise/Enums/EAkActionType.h"
#include "UE4/Wwise/WwiseFnv.h"
#include "UE4/Wwise/WwiseProvider.h"

using namespace CUE4Parse::FileProvider;
using namespace CUE4Parse::FileProvider::Vfs;
using CUE4Parse::UE4::Assets::Exports::Wwise::UAkAudioEvent;
using CUE4Parse::UE4::FMod::FModProvider;
using CUE4Parse::UE4::FMod::FModReader;
using CUE4Parse::UE4::FMod::FModSoundBank;
using CUE4Parse::UE4::Objects::Core::Misc::FGuid;
using CUE4Parse::UE4::Readers::FArchive;
using CUE4Parse::UE4::Readers::FByteArchive;
using CUE4Parse::UE4::Versions::VersionContainer;
using CUE4Parse::UE4::Wwise::WwiseExtractedSound;
using CUE4Parse::UE4::Wwise::WwiseProvider;
using CUE4Parse::Utils::StringComparer;
namespace WEn = CUE4Parse::UE4::Wwise::Enums;
namespace FMeta = CUE4Parse::UE4::FMod::Metadata;
namespace FUt = CUE4Parse::UE4::FMod::Utils;

static int g_failures = 0;
#define CHECK(cond)                                                           \
    do {                                                                      \
        if (!(cond)) {                                                        \
            std::cerr << "FAIL: " << #cond << " (line " << __LINE__ << ")\n"; \
            ++g_failures;                                                     \
        }                                                                     \
    } while (0)

// --------------------------------------------------------------------------------------------------
// byte writer
// --------------------------------------------------------------------------------------------------

struct Buf
{
    std::vector<uint8_t> Bytes;

    Buf& U8(uint8_t v) { Bytes.push_back(v); return *this; }
    Buf& U16(uint16_t v) { return Raw(&v, 2); }
    Buf& U32(uint32_t v) { return Raw(&v, 4); }
    Buf& I32(int32_t v) { return Raw(&v, 4); }
    Buf& I16(int16_t v) { return Raw(&v, 2); }
    Buf& U64(uint64_t v) { return Raw(&v, 8); }
    Buf& F32(float v) { return Raw(&v, 4); }
    Buf& Pad(int n) { for (int i = 0; i < n; i++) U8(0); return *this; }
    Buf& Tag(const char* t) { for (int i = 0; i < 4; i++) U8(static_cast<uint8_t>(t[i])); return *this; }
    Buf& Guid(uint32_t a, uint32_t b = 0, uint32_t c = 0, uint32_t d = 0) { return U32(a).U32(b).U32(c).U32(d); }
    Buf& EmptyList() { return U16(0); }   // FModReader's ReadX16-prefixed list with a zero count
    Buf& Cat(const Buf& other) { Bytes.insert(Bytes.end(), other.Bytes.begin(), other.Bytes.end()); return *this; }

    Buf& Raw(const void* p, int n)
    {
        const auto* b = static_cast<const uint8_t*>(p);
        Bytes.insert(Bytes.end(), b, b + n);
        return *this;
    }

    std::string AsString() const { return std::string(Bytes.begin(), Bytes.end()); }
};

// --------------------------------------------------------------------------------------------------
// pak writer — a minimal version-8 pak (see test_default_file_provider.cpp / test_config_ini.cpp)
// --------------------------------------------------------------------------------------------------

struct Writer
{
    std::vector<uint8_t> Bytes;

    int64_t Pos() const { return static_cast<int64_t>(Bytes.size()); }

    template <typename T>
    void Put(T value)
    {
        const auto* p = reinterpret_cast<const uint8_t*>(&value);
        Bytes.insert(Bytes.end(), p, p + sizeof(T));
    }

    void Raw(const std::string& data) { Bytes.insert(Bytes.end(), data.begin(), data.end()); }
    void Zeros(int64_t count) { Bytes.insert(Bytes.end(), static_cast<size_t>(count), 0); }

    void FString(const std::string& s)
    {
        Put<int32_t>(static_cast<int32_t>(s.size() + 1));
        Raw(s);
        Put<uint8_t>(0);
    }
};

static constexpr int32_t kStoredStructSize = 8 + 8 + 8 + 4 + 20 + 1 + 4;

struct PakFile
{
    std::string Path;
    std::string Content;
};

static std::vector<uint8_t> MakePak(const std::vector<PakFile>& files)
{
    Writer file;
    std::vector<int64_t> offsets;
    for (const auto& f : files)
    {
        offsets.push_back(file.Pos());
        file.Zeros(kStoredStructSize); // the duplicated record in front of the payload
        file.Raw(f.Content);
    }

    Writer index;
    index.FString("../../../Game/");
    index.Put<int32_t>(static_cast<int32_t>(files.size()));
    for (size_t i = 0; i < files.size(); ++i)
    {
        index.FString(files[i].Path);
        index.Put<int64_t>(offsets[i]);
        index.Put<int64_t>(static_cast<int64_t>(files[i].Content.size())); // compressed
        index.Put<int64_t>(static_cast<int64_t>(files[i].Content.size())); // uncompressed
        index.Put<int32_t>(0);  // method index: stored
        index.Zeros(20);        // hash
        index.Put<uint8_t>(0);  // not encrypted
        index.Put<uint32_t>(0); // compression block size
    }

    const int64_t indexOffset = file.Pos();
    file.Bytes.insert(file.Bytes.end(), index.Bytes.begin(), index.Bytes.end());

    file.Zeros(16);       // encryption key guid
    file.Put<uint8_t>(0); // the index is not encrypted
    file.Put<uint32_t>(CUE4Parse::UE4::Pak::Objects::FPakInfo::PAK_FILE_MAGIC);
    file.Put<int32_t>(8);
    file.Put<int64_t>(indexOffset);
    file.Put<int64_t>(index.Pos());
    file.Zeros(20); // IndexHash
    for (int i = 0; i < 5; ++i) file.Zeros(32); // empty compression-method names
    return file.Bytes;
}

class TestProvider : public AbstractVfsFileProvider
{
public:
    TestProvider() : AbstractVfsFileProvider(VersionContainer(), StringComparer::OrdinalIgnoreCase()) {}
    void Initialize() override {}
};

static std::shared_ptr<FArchive> Archive(std::string name, std::vector<uint8_t> bytes)
{
    return std::make_shared<FByteArchive>(std::move(name), std::move(bytes));
}

// --------------------------------------------------------------------------------------------------
// Wwise: a real v145 soundbank
// --------------------------------------------------------------------------------------------------

static const char* kEventName = "play_test";
static const uint32_t kMediaId = 42;
static const uint32_t kActionId = 0xA1;
static const uint32_t kFxId = 0xF1;
static const char* kWemBytes = "WEMBYTES"; // 8 bytes, the whole DATA section

static Buf Section(WEn::EChunkID id, const Buf& payload)
{
    Buf s;
    s.U32(static_cast<uint32_t>(id)).I32(static_cast<int32_t>(payload.Bytes.size())).Cat(payload);
    return s;
}

// One HIRC entry: type byte, 4-byte length, payload.
static Buf HircEntry(WEn::EAKBKHircType type, const Buf& payload)
{
    Buf e;
    e.U8(static_cast<uint8_t>(type)).U32(static_cast<uint32_t>(payload.Bytes.size())).Cat(payload);
    return e;
}

static std::string MakeSoundBank(uint32_t eventId)
{
    // BKHD, version 145: version, bank id, language, alt-values, project id, bank type, 16-byte hash.
    Buf header;
    header.U32(145).U32(0x1234).U32(0).U32(0).U32(0).U32(0).Pad(0x10);

    // DIDX: one 12-byte MediaHeader { id, offset, size } naming the whole DATA section.
    Buf didx;
    didx.U32(kMediaId).U32(0).I32(8);

    Buf data;
    data.Raw(kWemBytes, 8);

    // HIRC: an int32 count, then the entries.
    Buf hirc;
    hirc.I32(3);

    // Event -> one action. Above bank version 122 the action count is a 7-bit big-endian int.
    hirc.Cat(HircEntry(WEn::EAKBKHircType::Event, Buf().U32(eventId).U8(1).U32(kActionId)));

    // EventAction(Play) -> the FX node. Payload after the prop bundle is CAkActionPlay: a one-byte
    // CAkActionParams bit vector, the bank id, and (>= 144) the bank type.
    hirc.Cat(HircEntry(WEn::EAKBKHircType::EventAction,
        Buf().U32(kActionId)
             .U8(0).U8(static_cast<uint8_t>(WEn::EAkActionType::Play))
             .U32(kFxId)
             .U8(0)          // isBus
             .U8(0).U8(0)    // empty prop bundle (props, ranges)
             .U8(0).U32(0x1234).U32(0)));

    // FxCustom -> media id 42. The plugin word has its top bit set, which is what makes
    // WwisePlugin::TryParsePluginParams bail before reading a parameter blob.
    hirc.Cat(HircEntry(WEn::EAKBKHircType::FxCustom,
        Buf().U32(kFxId).U32(0x80000001u)
             .U8(1).U8(0).U32(kMediaId)   // one AkMediaMap { index, source id }
             .U16(0)                      // no RTPCs
             .U8(0).U8(0)                 // AkStateAwareChunk: no property infos, no groups
             .U16(0)));                   // no plugin property values

    Buf bank;
    bank.Cat(Section(WEn::EChunkID::BankHeader, header));
    bank.Cat(Section(WEn::EChunkID::BankDataIndex, didx));
    bank.Cat(Section(WEn::EChunkID::BankData, data));
    bank.Cat(Section(WEn::EChunkID::BankHierarchy, hirc));
    return bank.AsString();
}

static void TestWwiseProviderResolvesAnEventToItsMedia()
{
    const uint32_t eventId = CUE4Parse::UE4::Wwise::WwiseFnv::GetHash(kEventName);

    TestProvider provider;
    provider.RegisterVfs(Archive("pakchunk0.pak", MakePak({
        {"Content/WwiseAudio/Test.bnk", MakeSoundBank(eventId)},
    })));
    CHECK(provider.Mount() == 1);
    CHECK(provider.Files.Count() == 1);

    // The constructor bulk-loads every bank it can see and throws when that found nothing, so reaching
    // the next line at all means BulkInitializeWwise parsed the bank and CacheWwiseFile indexed it.
    WwiseProvider wwise(provider, "");

    UAkAudioEvent audioEvent;
    audioEvent.Name = kEventName;
    // No EventCookedData and no ShortID property, so the id is the FNV hash of the name -- the older
    // cooking path, and the one that goes through the hierarchy rather than reading the media list.
    const std::vector<WwiseExtractedSound> sounds = wwise.ExtractAudioEventSounds(audioEvent);

    CHECK(sounds.size() == 1);
    if (sounds.size() != 1) return;

    // Event -> EventAction(Play) -> FxCustom -> media 42, named "<debug name> (<id>)".
    CHECK(sounds[0].Extension == "wem");
    CHECK(sounds[0].OutputPath.find("play_test (42)") != std::string::npos);
    CHECK(sounds[0].ToString() == sounds[0].OutputPath + ".wem");

    // The bytes are fetched now, through the deferred read this whole layer is built on: a sub-range of
    // the pak entry, resolved lazily long after the bank was parsed.
    const std::vector<uint8_t> bytes = sounds[0].GetData();
    CHECK(bytes.size() == 8);
    CHECK(bytes.size() == 8 && std::memcmp(bytes.data(), kWemBytes, 8) == 0);
}

static void TestWwiseProviderUnknownEventResolvesToNothing()
{
    TestProvider provider;
    provider.RegisterVfs(Archive("pakchunk0.pak", MakePak({
        {"Content/WwiseAudio/Test.bnk",
         MakeSoundBank(CUE4Parse::UE4::Wwise::WwiseFnv::GetHash(kEventName))},
    })));
    CHECK(provider.Mount() == 1);

    WwiseProvider wwise(provider, "");

    // A name that hashes to an id no hierarchy carries walks zero nodes and saves nothing -- it must not
    // fall back to "everything in the bank".
    UAkAudioEvent audioEvent;
    audioEvent.Name = "some_other_event";
    CHECK(wwise.ExtractAudioEventSounds(audioEvent).empty());
}

static void TestWwiseProviderThrowsWithNothingToLoad()
{
    // C# throws InvalidOperationException when the bulk init loaded no bank at all; the port throws too.
    TestProvider provider;
    provider.RegisterVfs(Archive("pakchunk0.pak", MakePak({{"Content/Readme.txt", "nothing to see"}})));
    CHECK(provider.Mount() == 1);

    bool threw = false;
    try { WwiseProvider wwise(provider, ""); }
    catch (const std::exception&) { threw = true; }
    CHECK(threw);
}

static void TestWwiseExtractedSoundToString()
{
    // ToString lower-cases the extension, as C# does, and does not touch the path.
    WwiseExtractedSound sound{"Audio/Some Sound", "WEM", nullptr};
    CHECK(sound.ToString() == "Audio/Some Sound.wem");
    CHECK(sound.GetData().empty()); // a null Data is "no bytes", not a crash
}

// --------------------------------------------------------------------------------------------------
// FMOD: the synthetic "FEV " bank from test_fmod_bank.cpp, mounted under Content/FMOD
// --------------------------------------------------------------------------------------------------

static const uint32_t GID_EVENT = 0xE0;
static const uint32_t GID_TML   = 0x70;
static const uint32_t GID_PML   = 0x60;
static const uint32_t GID_INST1 = 0x11, GID_INST2 = 0x12, GID_INST3 = 0x13;
static const uint32_t GID_WAV1  = 0x21, GID_WAV2  = 0x22, GID_WAV3  = 0x23;
static const uint32_t GID_BANK  = 0xB4A4;

static Buf FModChunk(const char* tag, const Buf& payload)
{
    Buf c;
    c.Tag(tag).U32(static_cast<uint32_t>(payload.Bytes.size())).Cat(payload);
    return c;
}

// A v0x83 bank wiring one event to three waveforms by three different routes.
static std::string MakeFModBank()
{
    Buf chunks;
    chunks.Cat(FModChunk("FMT ", Buf().I32(0x83).I32(0x83)));
    chunks.Cat(FModChunk("BNKI", Buf().Guid(GID_BANK).U64(0xABCD).I32(1).I32(0)));

    {
        Buf p;
        p.Guid(GID_EVENT).Guid(0).Guid(GID_TML).Guid(0).Guid(0);
        p.I32(16).I32(1).U8(1).I32(2);
        p.I16(2).U16(16).Guid(GID_PML);
        p.EmptyList().EmptyList();
        p.F32(1.0f).I32(1).F32(0.25f).U32(0x8000);
        p.EmptyList().EmptyList();
        p.I16(2).U16(16).Guid(GID_INST2);
        chunks.Cat(FModChunk("EVTB", p));
    }
    {
        Buf p;
        p.Guid(GID_TML);
        p.I16(2).U16(24).Guid(GID_INST1).U32(0).U32(1000);
        p.EmptyList().EmptyList().EmptyList().EmptyList();
        chunks.Cat(FModChunk("TLNB", p));
    }
    chunks.Cat(FModChunk("PMLB", Buf().Guid(GID_PML).Guid(0).I16(2).U16(16).Guid(GID_INST3).U32(0)));

    chunks.Cat(FModChunk("WAIB", Buf().Guid(GID_INST1).Guid(GID_WAV1)));
    chunks.Cat(FModChunk("WAIB", Buf().Guid(GID_INST2).Guid(GID_WAV2)));
    chunks.Cat(FModChunk("WAIB", Buf().Guid(GID_INST3).Guid(GID_WAV3)));

    chunks.Cat(FModChunk("WAV ", Buf().Guid(GID_WAV1).U16(16).I32(0).I32(0).U32(0)));
    chunks.Cat(FModChunk("WAV ", Buf().Guid(GID_WAV2).U16(16).I32(0).I32(1).U32(0)));
    chunks.Cat(FModChunk("WAV ", Buf().Guid(GID_WAV3).U16(16).I32(0).I32(2).U32(0)));

    Buf bank;
    bank.Tag("RIFF").U32(static_cast<uint32_t>(chunks.Bytes.size() + 4)).Tag("FEV ").Cat(chunks);
    return bank.AsString();
}

static void ResetFModStatics()
{
    FModReader::FormatInfo = FMeta::FFormatInfo();
    FModReader::SoundDataInfo.reset();
    FModReader::EncryptionKey.reset();
}

static void TestFModProviderDiscoversAndMergesPakBanks()
{
    ResetFModStatics();

    TestProvider provider;
    provider.RegisterVfs(Archive("pakchunk0.pak", MakePak({
        // Two files of the same logical bank: FModProvider groups by the name up to the first '.' and
        // merges them into one reader before keying the cache by bank guid.
        {"Content/FMOD/Desktop/Master.bank", MakeFModBank()},
        {"Content/FMOD/Desktop/Master.assets.bank", MakeFModBank()},
        // Not under an FMOD directory, so it must be ignored entirely.
        {"Content/Audio/Elsewhere.bank", MakeFModBank()},
    })));
    CHECK(provider.Mount() == 1);
    CHECK(provider.Files.Count() == 3);

    // gameDirectory is not a "Paks" folder, so LoadFileBanks bails before touching the disk and only the
    // pak route runs -- which is what this test is about.
    FModProvider fmod(provider, "");

    // Both Master files merged into a single reader under one guid; Elsewhere.bank never entered.
    CHECK(fmod.MergedReaders().size() == 1);

    const FModReader* merged = fmod.MergedReaders().empty() ? nullptr
                                                            : fmod.MergedReaders().begin()->second.get();
    CHECK(merged != nullptr);
    if (merged == nullptr) return;
    CHECK(merged->EventNodes.size() == 1);
    CHECK(merged->WavEntries.size() == 3);

    // The bank carries no SND chunk, so nothing decodes and the event resolves to no waveforms. That must
    // come back as an empty list rather than the whole sound table (which is what the "all waveforms
    // resolved" guard in ExtractEventSounds is for).
    CHECK(fmod.ExtractBankSounds(*merged).empty());
    CHECK(fmod.ExtractBankSoundTable(*merged).empty()); // no STBL chunk either
}

static void TestFModProviderExtractsAgainstASoundBank()
{
    ResetFModStatics();
    FModReader::FormatInfo.FileVersion = 0x83;
    FModReader::FormatInfo.CompatVersion = 0x83;

    TestProvider provider;
    provider.RegisterVfs(Archive("pakchunk0.pak", MakePak({
        {"Content/FMOD/Desktop/Master.bank", MakeFModBank()},
    })));
    CHECK(provider.Mount() == 1);
    FModProvider fmod(provider, "");

    // A reader built outside the provider so the FSB5 container stub can be injected -- the real decode is
    // out of scope (see FModSoundBank.h), and a sample count is all the extraction path needs.
    const std::string bankBytes = MakeFModBank();
    FByteArchive Ar("synthetic.bank", std::vector<uint8_t>(bankBytes.begin(), bankBytes.end()));
    FModReader reader(Ar, "synthetic.bank");

    FModSoundBank soundBank;
    soundBank.SampleCount = 3;
    reader.SoundBankData.push_back(soundBank);

    CHECK(reader.ExtractTracks().size() == 3);

    const auto sounds = fmod.ExtractBankSounds(reader);
    CHECK(sounds.size() == 3);
    if (sounds.size() == 3)
    {
        // C#'s `sample.Name ?? $"{fallback}_{i}"` -- the names live in the FSB5 name table the decoder
        // reads, so the fallback is what survives the port.
        CHECK(sounds[0].Name == "synthetic.bank_0");
        CHECK(sounds[2].Name == "synthetic.bank_2");
        CHECK(sounds[1].Waveform.SoundBankIndex == 0 && sounds[1].Waveform.SubsoundIndex == 1);
        CHECK(sounds[1].SoundBank == &reader.SoundBankData[0]);
        CHECK(sounds[2].ToString() == "synthetic.bank_2");
    }

    // A subsound index past the container's sample count is dropped, exactly where C# drops a sample whose
    // RebuildAsStandardFileFormat failed.
    reader.SoundBankData[0].SampleCount = 2;
    CHECK(fmod.ExtractBankSounds(reader).size() == 2);
    reader.SoundBankData.clear();
    CHECK(fmod.ExtractBankSounds(reader).empty());
}

static void TestFModProviderWithNoBanksIsInert()
{
    ResetFModStatics();

    TestProvider provider;
    provider.RegisterVfs(Archive("pakchunk0.pak", MakePak({{"Content/Readme.txt", "no banks here"}})));
    CHECK(provider.Mount() == 1);

    // Unlike WwiseProvider, FModProvider does not throw on an empty game -- it just has nothing to say.
    FModProvider fmod(provider, "");
    CHECK(fmod.MergedReaders().empty());
}

int main()
{
    try
    {
        TestWwiseProviderResolvesAnEventToItsMedia();
        TestWwiseProviderUnknownEventResolvesToNothing();
        TestWwiseProviderThrowsWithNothingToLoad();
        TestWwiseExtractedSoundToString();
        TestFModProviderDiscoversAndMergesPakBanks();
        TestFModProviderExtractsAgainstASoundBank();
        TestFModProviderWithNoBanksIsInert();
    }
    catch (const std::exception& e)
    {
        std::cerr << "EXCEPTION: " << e.what() << "\n";
        return 2;
    }

    if (g_failures == 0)
    {
        std::cout << "test_audio_providers: all checks passed\n";
        return 0;
    }
    std::cerr << g_failures << " check(s) failed.\n";
    return 1;
}
