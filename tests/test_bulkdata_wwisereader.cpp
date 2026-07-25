// Tests the bulk-data / payload layer and the Wwise container reader built on it.
//
// As with the other suites in this tree, including every new header at once is a real part of the test:
// most of these are header-only, so a header nothing includes is never compiled. The list doubles as an
// inventory of the slice.
//
// The behavioural half pins the things a mechanical C#-to-C++ translation gets wrong here:
//   * FByteBulkDataHeader has three completely different readings (IoPackage bulk-data map, classic
//     package data-resource map, inline) and the map arms must give the 4 index bytes back on a miss;
//   * the BULKDATA_Size64Bit / BULKDATA_AT_LARGE_OFFSETS width switches;
//   * TBulkData's laziness: nothing is read until Data() is touched, and the archive cursor after the
//     constructor depends on which payload-location flag is set;
//   * FAssetArchive's payload registry: adding the same type twice throws, TryGetPayload swallows;
//   * WwiseReader's section walk always re-seeks to the declared end of each section;
//   * ReadDeferredByteData's three source kinds, and that all three leave the cursor past the range;
//   * PropertyUtil's coercion arms, including the signedness rule and the enum fallback.
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "UE4/Assets/Objects/EBulkDataFlags.h"
#include "UE4/Assets/Objects/FByteBulkData.h"
#include "UE4/Assets/Objects/FByteBulkDataHeader.h"
#include "UE4/Assets/Objects/FColorBulkData.h"
#include "UE4/Assets/Objects/FIntBulkData.h"
#include "UE4/Assets/Objects/TBulkData.h"
#include "UE4/Assets/Utils/PayloadType.h"
#include "UE4/Objects/UObject/FObjectDataResource.h"
#include "UE4/Assets/Exports/PropertyUtil.h"
#include "UE4/Wwise/FDeferredByteData.h"
#include "UE4/Wwise/WwiseReader.h"
#include "UE4/Wwise/Objects/AkEntry.h"

// The export types this slice added, included purely so they are compiled.
#include "UE4/Assets/Exports/Wwise/FAkMediaDataChunk.h"
#include "UE4/Assets/Exports/Wwise/FWwiseAssetLibraryCookedData.h"
#include "UE4/Assets/Exports/Wwise/FWwiseAuxBusCookedData.h"
#include "UE4/Assets/Exports/Wwise/FWwiseEventCookedData.h"
#include "UE4/Assets/Exports/Wwise/FWwiseExternalSourceCookedData.h"
#include "UE4/Assets/Exports/Wwise/FWwiseGroupValueCookedData.h"
#include "UE4/Assets/Exports/Wwise/FWwiseInitBankCookedData.h"
#include "UE4/Assets/Exports/Wwise/FWwiseLanguageCookedData.h"
#include "UE4/Assets/Exports/Wwise/FWwiseLocalizedAuxBusCookedData.h"
#include "UE4/Assets/Exports/Wwise/FWwiseLocalizedCookedDataMap.h"
#include "UE4/Assets/Exports/Wwise/FWwiseLocalizedEventCookedData.h"
#include "UE4/Assets/Exports/Wwise/FWwiseLocalizedShareSetCookedData.h"
#include "UE4/Assets/Exports/Wwise/FWwiseLocalizedSoundBankCookedData.h"
#include "UE4/Assets/Exports/Wwise/FWwiseMediaCookedData.h"
#include "UE4/Assets/Exports/Wwise/FWwisePackagedFile.h"
#include "UE4/Assets/Exports/Wwise/FWwiseShareSetCookedData.h"
#include "UE4/Assets/Exports/Wwise/FWwiseSoundBankCookedData.h"
#include "UE4/Assets/Exports/Wwise/FWwiseSwitchContainerLeafCookedData.h"
#include "UE4/Assets/Exports/Wwise/UAkAcousticTexture.h"
#include "UE4/Assets/Exports/Wwise/UAkAssetData.h"
#include "UE4/Assets/Exports/Wwise/UAkAssetDataSwitchContainer.h"
#include "UE4/Assets/Exports/Wwise/UAkAssetDataWithMedia.h"
#include "UE4/Assets/Exports/Wwise/UAkAssetPlatformData.h"
#include "UE4/Assets/Exports/Wwise/UAkAudioBank.h"
#include "UE4/Assets/Exports/Wwise/UAkAudioDeviceShareSet.h"
#include "UE4/Assets/Exports/Wwise/UAkAudioEvent.h"
#include "UE4/Assets/Exports/Wwise/UAkAudioEventData.h"
#include "UE4/Assets/Exports/Wwise/UAkAudioType.h"
#include "UE4/Assets/Exports/Wwise/UAkAuxBus.h"
#include "UE4/Assets/Exports/Wwise/UAkEffectShareSet.h"
#include "UE4/Assets/Exports/Wwise/UAkExternalMediaAsset.h"
#include "UE4/Assets/Exports/Wwise/UAkGroupValue.h"
#include "UE4/Assets/Exports/Wwise/UAkInitBank.h"
#include "UE4/Assets/Exports/Wwise/UAkInitBankAssetData.h"
#include "UE4/Assets/Exports/Wwise/UAkLocalizedMediaAsset.h"
#include "UE4/Assets/Exports/Wwise/UAkMediaAsset.h"
#include "UE4/Assets/Exports/Wwise/UAkMediaAssetData.h"
#include "UE4/Assets/Exports/Wwise/UAkRtpc.h"
#include "UE4/Assets/Exports/Wwise/UAkStateValue.h"
#include "UE4/Assets/Exports/Wwise/UAkSwitchValue.h"
#include "UE4/Assets/Exports/Wwise/UAkTrigger.h"
#include "UE4/Assets/Exports/Wwise/UWwiseAssetLibrary.h"
#include "UE4/Assets/Exports/FMod/UFMODBank.h"
#include "UE4/Assets/Exports/FMod/UFMODBankLookup.h"
#include "UE4/Assets/Exports/FMod/UFMODBus.h"
#include "UE4/Assets/Exports/FMod/UFMODEvent.h"
#include "UE4/Assets/Exports/FMod/UFMODSnapshot.h"
#include "UE4/Assets/Exports/FMod/UFMODSnapshotReverb.h"
#include "UE4/Assets/Exports/FMod/UFMODVCA.h"

#include "UE4/Assets/IPackage.h"
#include "UE4/Objects/UObject/FPackageFileSummary.h"
#include "UE4/Assets/Objects/Properties/IntProperty.h"
#include "UE4/Readers/FByteArchive.h"

namespace Obj = CUE4Parse::UE4::Assets::Objects;
namespace Rd = CUE4Parse::UE4::Readers;
namespace Ar_ = CUE4Parse::UE4::Assets::Readers;
namespace Ww = CUE4Parse::UE4::Wwise;
namespace PU = CUE4Parse::UE4::Assets::Exports::PropertyUtil;

using CUE4Parse::UE4::Assets::Objects::EBulkDataFlags;
using CUE4Parse::UE4::Assets::Utils::PayloadType;

static int g_failures = 0;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            std::cerr << "FAIL: " << #cond << " (line " << __LINE__ << ")\n"; \
            ++g_failures;                                                  \
        }                                                                  \
    } while (0)

// ---------------------------------------------------------------- byte-buffer helper

// Little-endian writer, matching the format on disk.
struct Buf
{
    std::vector<uint8_t> Bytes;

    Buf& U8(uint8_t v) { Bytes.push_back(v); return *this; }
    Buf& U16(uint16_t v) { for (int i = 0; i < 2; i++) Bytes.push_back(static_cast<uint8_t>(v >> (8 * i))); return *this; }
    Buf& U32(uint32_t v) { for (int i = 0; i < 4; i++) Bytes.push_back(static_cast<uint8_t>(v >> (8 * i))); return *this; }
    Buf& I32(int32_t v) { return U32(static_cast<uint32_t>(v)); }
    Buf& U64(uint64_t v) { for (int i = 0; i < 8; i++) Bytes.push_back(static_cast<uint8_t>(v >> (8 * i))); return *this; }
    Buf& F32(float v) { uint32_t bits; std::memcpy(&bits, &v, 4); return U32(bits); }
    Buf& Raw(const std::vector<uint8_t>& v) { Bytes.insert(Bytes.end(), v.begin(), v.end()); return *this; }
    Buf& Pad(size_t n) { Bytes.insert(Bytes.end(), n, 0); return *this; }
    size_t Size() const { return Bytes.size(); }
};

// A minimal IPackage so an FAssetArchive can be built without a real package on disk. The bulk-data
// header's two map arms both check the concrete package type, so a bare IPackage takes the inline path --
// which is exactly the arm most of these tests want.
class FakePackage final : public CUE4Parse::UE4::Assets::IPackage
{
public:
    CUE4Parse::UE4::Objects::UObject::FPackageFileSummary Summary;
    std::vector<CUE4Parse::UE4::Objects::UObject::FNameEntrySerialized> Names;

    const std::string& GetName() const override { return _name; }
    const std::vector<CUE4Parse::UE4::Objects::UObject::FNameEntrySerialized>& NameMap() const override { return Names; }
    bool HasFlags(CUE4Parse::UE4::Objects::UObject::EPackageFlags) const override { return false; }
    const CUE4Parse::UE4::Objects::UObject::FPackageFileSummary* GetSummary() const override { return &Summary; }
    CUE4Parse::UE4::Assets::ResolvedObject* ResolvePackageIndex(
        const CUE4Parse::UE4::Objects::UObject::FPackageIndex*) override { return nullptr; }
    CUE4Parse::UE4::Assets::Exports::UObject* GetExportObject(int) override { return nullptr; }
    CUE4Parse::FileProvider::IFileProvider* GetProvider() const override { return nullptr; }
    int GetExportIndex(const std::string&) const override { return -1; }

private:
    std::string _name = "FakePackage";
};

// ---------------------------------------------------------------- tests

static void TestPayloadTypeExtensions()
{
    CHECK(std::string(CUE4Parse::UE4::Assets::Utils::ToExtension(PayloadType::UBULK)) == "ubulk");
    CHECK(std::string(CUE4Parse::UE4::Assets::Utils::ToExtension(PayloadType::UPTNL)) == "uptnl");
    // MUBULK's real sidecar is ".m.ubulk"; this is only the enumerator spelling, as in C#.
    CHECK(std::string(CUE4Parse::UE4::Assets::Utils::ToExtension(PayloadType::MUBULK)) == "mubulk");
}

static void TestBulkDataFlagsHelpers()
{
    const auto both = EBulkDataFlags::BULKDATA_PayloadInSeperateFile | EBulkDataFlags::BULKDATA_MemoryMappedPayload;
    // HasFlag is an ALL-bits test, which is what makes the memory-mapped arm distinguishable from the
    // plain separate-file arm in TBulkData::GetBulkArchive.
    CHECK(HasFlag(both, EBulkDataFlags::BULKDATA_PayloadInSeperateFile));
    CHECK(HasFlag(both, both));
    CHECK(!HasFlag(EBulkDataFlags::BULKDATA_PayloadInSeperateFile, both));
}

static void TestInlineHeaderWidths()
{
    // 32-bit widths: flags, element count, size on disk, then a 32-bit offset for an old package version.
    FakePackage pkg;
    pkg.Summary.BulkDataStartOffset = 0;
    Buf b;
    b.U32(static_cast<uint32_t>(EBulkDataFlags::BULKDATA_ForceInlinePayload | EBulkDataFlags::BULKDATA_NoOffsetFixUp))
     .I32(16).U32(16).U64(4096);

    Rd::FByteArchive base("t.uasset", b.Bytes);
    Ar_::FAssetArchive Ar(base, &pkg);
    const Obj::FByteBulkDataHeader h(Ar);

    CHECK(h.ElementCount == 16);
    CHECK(h.SizeOnDisk == 16u);
    CHECK(h.OffsetInFile == 4096);           // modern packages read a 64-bit offset
    CHECK(h.CookedIndex.IsDefault());
}

static void TestOffsetFixUpAppliesBulkDataStart()
{
    // Without BULKDATA_NoOffsetFixUp the summary's BulkDataStartOffset is added to the stored offset.
    FakePackage pkg;
    pkg.Summary.BulkDataStartOffset = 1000;
    Buf b;
    b.U32(static_cast<uint32_t>(EBulkDataFlags::BULKDATA_ForceInlinePayload)).I32(4).U32(4).U64(24);

    Rd::FByteArchive base("t.uasset", b.Bytes);
    Ar_::FAssetArchive Ar(base, &pkg);
    const Obj::FByteBulkDataHeader h(Ar);

    CHECK(h.OffsetInFile == 1024);
}

static void TestSize64BitWidensBothCounts()
{
    FakePackage pkg;
    Buf b;
    b.U32(static_cast<uint32_t>(EBulkDataFlags::BULKDATA_Size64Bit | EBulkDataFlags::BULKDATA_NoOffsetFixUp))
     .U64(9).U64(9).U64(64);

    Rd::FByteArchive base("t.uasset", b.Bytes);
    Ar_::FAssetArchive Ar(base, &pkg);
    const Obj::FByteBulkDataHeader h(Ar);

    CHECK(h.ElementCount == 9);
    CHECK(h.SizeOnDisk == 9u);
    CHECK(h.OffsetInFile == 64);
    CHECK(Ar.Position == 28); // 4 flags + 8 + 8 + 8: all three widened
}

static void TestBadDataVersionSkipsTwoBytesAndClearsFlag()
{
    FakePackage pkg;
    Buf b;
    b.U32(static_cast<uint32_t>(EBulkDataFlags::BULKDATA_BadDataVersion | EBulkDataFlags::BULKDATA_NoOffsetFixUp))
     .I32(1).U32(1).U64(0).U16(0xFFFF);

    Rd::FByteArchive base("t.uasset", b.Bytes);
    Ar_::FAssetArchive Ar(base, &pkg);
    const Obj::FByteBulkDataHeader h(Ar);

    // The flag is cleared after the skip, so downstream flag tests do not see it.
    CHECK(!HasFlag(h.BulkDataFlags, EBulkDataFlags::BULKDATA_BadDataVersion));
    CHECK(Ar.Position == 22); // 20 header bytes + the 2 skipped
}

static void TestInlinePayloadIsLazyAndSkipped()
{
    FakePackage pkg;
    Buf b;
    b.U32(static_cast<uint32_t>(EBulkDataFlags::BULKDATA_ForceInlinePayload | EBulkDataFlags::BULKDATA_NoOffsetFixUp))
     .I32(4).U32(4).U64(0)
     .U8(0xDE).U8(0xAD).U8(0xBE).U8(0xEF)
     .U32(0x11223344); // a marker after the payload

    Rd::FByteArchive base("t.uasset", b.Bytes);
    Ar_::FAssetArchive Ar(base, &pkg);
    Obj::FByteBulkData bulk(Ar);

    // The constructor steps past an inline payload so the caller can carry on reading.
    CHECK(Ar.Position == 24);
    CHECK(Ar.Read<uint32_t>() == 0x11223344u);

    // ...and only now are the bytes actually fetched.
    const std::vector<uint8_t>* data = bulk.Data();
    CHECK(data != nullptr);
    if (data != nullptr)
    {
        CHECK(data->size() == 4);
        CHECK((*data)[0] == 0xDE && (*data)[3] == 0xEF);
    }
    CHECK(bulk.GetDataSize() == 4); // FByteBulkData overrides this to the element count, not count*sizeof(T)
}

static void TestZeroSizeBulkDataReadsEmptyNotNull()
{
    FakePackage pkg;
    Buf b;
    b.U32(static_cast<uint32_t>(EBulkDataFlags::BULKDATA_NoOffsetFixUp)).I32(0).U32(0).U64(0);

    Rd::FByteArchive base("t.uasset", b.Bytes);
    Ar_::FAssetArchive Ar(base, &pkg);
    Obj::FByteBulkData bulk(Ar);

    // C# hands back an empty array here rather than null -- the distinction matters to every caller that
    // treats null as "the read failed".
    const std::vector<uint8_t>* data = bulk.Data();
    CHECK(data != nullptr);
    if (data != nullptr) CHECK(data->empty());
}

static void TestPayloadAtEndOfFileSeeks()
{
    FakePackage pkg;
    Buf b;
    b.U32(static_cast<uint32_t>(EBulkDataFlags::BULKDATA_PayloadAtEndOfFile | EBulkDataFlags::BULKDATA_NoOffsetFixUp))
     .I32(3).U32(3).U64(24)
     .Pad(4)                       // filler between the header and the payload
     .U8(1).U8(2).U8(3);

    Rd::FByteArchive base("t.uasset", b.Bytes);
    Ar_::FAssetArchive Ar(base, &pkg);
    Obj::FByteBulkData bulk(Ar);

    // Not inline, so the cursor does NOT move past a payload...
    CHECK(Ar.Position == 20);
    // ...but the read still lands at OffsetInFile.
    const std::vector<uint8_t>* data = bulk.Data();
    CHECK(data != nullptr);
    if (data != nullptr && data->size() == 3)
    {
        CHECK((*data)[0] == 1 && (*data)[1] == 2 && (*data)[2] == 3);
    }
    else CHECK(false);
}

static void TestSeparateFilePayloadFailsWithoutRegistration()
{
    // No payload was registered for UBULK, so the lookup fails and Data() reports null rather than
    // silently returning whatever happened to be at that offset in the uasset.
    FakePackage pkg;
    Buf b;
    b.U32(static_cast<uint32_t>(EBulkDataFlags::BULKDATA_PayloadInSeperateFile | EBulkDataFlags::BULKDATA_NoOffsetFixUp))
     .I32(4).U32(4).U64(0);

    Rd::FByteArchive base("t.uasset", b.Bytes);
    Ar_::FAssetArchive Ar(base, &pkg);
    Obj::FByteBulkData bulk(Ar);

    CHECK(bulk.Data() == nullptr);
}

static void TestPayloadRegistryRoundTrip()
{
    FakePackage pkg;
    std::vector<uint8_t> uassetBytes(64, 0);
    Rd::FByteArchive base("t.uasset", uassetBytes);
    Ar_::FAssetArchive Ar(base, &pkg);

    CHECK(Ar.TryGetPayload(PayloadType::UBULK) == nullptr); // nothing registered yet

    std::vector<uint8_t> ubulkBytes{9, 8, 7, 6};
    Ar.AddPayload(PayloadType::UBULK, 0,
        [ubulkBytes](const Obj::FByteBulkDataHeader*) -> std::unique_ptr<Rd::FArchive>
        { return std::make_unique<Rd::FByteArchive>("t.ubulk", ubulkBytes); });

    auto payload = Ar.TryGetPayload(PayloadType::UBULK);
    CHECK(payload != nullptr);
    if (payload != nullptr)
    {
        CHECK(payload->Length == 4);
        CHECK(payload->Read<uint8_t>() == 9);
    }

    // Registering the same type twice is an error in C# and here.
    bool threw = false;
    try
    {
        Ar.AddPayload(PayloadType::UBULK, 0,
            [](const Obj::FByteBulkDataHeader*) -> std::unique_ptr<Rd::FArchive> { return nullptr; });
    }
    catch (...) { threw = true; }
    CHECK(threw);

    // TryGetPayload swallows the "not found" throw that GetPayload raises.
    CHECK(Ar.TryGetPayload(PayloadType::MUBULK) == nullptr);
    bool getThrew = false;
    try { (void) Ar.GetPayload(PayloadType::MUBULK); } catch (...) { getThrew = true; }
    CHECK(getThrew);
}

static void TestPayloadMapIsSharedWithClones()
{
    FakePackage pkg;
    std::vector<uint8_t> uassetBytes(16, 0);
    Rd::FByteArchive base("t.uasset", uassetBytes);
    Ar_::FAssetArchive Ar(base, &pkg);

    std::vector<uint8_t> ubulkBytes{42};
    Ar.AddPayload(PayloadType::UBULK, 0,
        [ubulkBytes](const Obj::FByteBulkDataHeader*) -> std::unique_ptr<Rd::FArchive>
        { return std::make_unique<Rd::FByteArchive>("t.ubulk", ubulkBytes); });

    // C# deliberately carries the payload dictionary over to clones; so does the port.
    auto clone = Ar.Clone();
    auto* assetClone = dynamic_cast<Ar_::FAssetArchive*>(clone.get());
    CHECK(assetClone != nullptr);
    if (assetClone != nullptr) CHECK(assetClone->TryGetPayload(PayloadType::UBULK) != nullptr);
}

static void TestDeferredByteDataKinds()
{
    // FArrayDeferredByteData: the bytes are already in hand.
    Ww::FArrayDeferredByteData present(std::vector<uint8_t>{1, 2, 3});
    CHECK(present.IsValid());
    CHECK(present.LoadedSize() == 3);
    CHECK(present.GetData().size() == 3);

    Ww::FArrayDeferredByteData empty{std::vector<uint8_t>{}};
    CHECK(!empty.IsValid());          // an empty array is NOT valid, matching C#'s `{Length: > 0}`
    CHECK(empty.LoadedSize() == 0);

    // A game-file source with no file is not valid either.
    Ww::FGameFileDeferredByteData noFile(nullptr);
    CHECK(!noFile.IsValid());
    CHECK(noFile.GetData().empty());
}

static void TestReadDeferredByteDataFallsBackToCopy()
{
    // With the plain Archive source there is nothing to defer to, so the bytes are copied out -- and the
    // cursor still ends up past the range, exactly as the two deferring arms leave it.
    Buf b;
    b.Pad(4).U8(0xAA).U8(0xBB).U8(0xCC).U8(0xDD).Pad(4);
    Rd::FByteArchive Ar("t.bnk", b.Bytes);

    const auto source = Ww::WwiseDataSource::Archive();
    auto data = Ww::WwiseReader::ReadDeferredByteData(Ar, source, 4, 4);

    CHECK(data != nullptr);
    if (data != nullptr)
    {
        CHECK(data->IsValid());
        CHECK(data->LoadedSize() == 4);
        const auto bytes = data->GetData();
        CHECK(bytes.size() == 4 && bytes[0] == 0xAA && bytes[3] == 0xDD);
    }
    CHECK(Ar.Position == 8);
}

static void TestTryReadSoundBankId()
{
    // A BankHeader section: tag, length, then version + soundbank id.
    Buf b;
    b.U32(static_cast<uint32_t>(Ww::Enums::EChunkID::BankHeader)).I32(8).U32(140).U32(0xCAFEBABE);
    Rd::FByteArchive Ar("t.bnk", b.Bytes);

    const auto id = Ww::WwiseReader::TryReadSoundBankId(Ar);
    CHECK(id.has_value());
    if (id.has_value()) CHECK(*id == 0xCAFEBABEu);

    // A bank with no header section yields nothing rather than a bogus id.
    Buf other;
    other.U32(static_cast<uint32_t>(Ww::Enums::EChunkID::FXPR)).I32(4).U32(0);
    Rd::FByteArchive otherAr("t.bnk", other.Bytes);
    CHECK(!Ww::WwiseReader::TryReadSoundBankId(otherAr).has_value());
}

static void TestWwiseReaderSkipsUnknownSections()
{
    // Two sections: one the reader does not handle, then a BankHeader. If the walk did not re-seek to the
    // declared end of the first section it would read the header at the wrong offset.
    // A v140 BankHeader is 0x14 bytes: version, id, languageId, alt-values flags, project id.
    Buf b;
    b.U32(0x5A5A5A5Au).I32(6).Pad(6)                                  // unknown section, 6 bytes of payload
     .U32(static_cast<uint32_t>(Ww::Enums::EChunkID::BankHeader)).I32(0x14)
     .U32(140).U32(0x12345678).U32(0).U32(0).U32(0);

    Ww::FWwiseArchive Ar("t.bnk", b.Bytes);
    const auto source = Ww::WwiseDataSource::Archive();
    const Ww::WwiseReader reader(Ar, source);

    CHECK(reader.Header.Version == 140u);
    CHECK(reader.Header.SoundBankId == 0x12345678u);
    CHECK(Ar.Version == 140u); // the header's version is threaded back into the archive
    CHECK(reader.TotalSize == static_cast<int64_t>(b.Size()));
}

static void TestWwiseReaderMidiSectionRewindsEight()
{
    // MIDI (like RIFF and PLUGIN) backs up over the tag+length so the deferred range covers the whole
    // section, header included -- that is why the size is `8 + sectionLength`.
    Buf b;
    b.U32(static_cast<uint32_t>(Ww::Enums::EChunkID::MIDI)).I32(4).U8(1).U8(2).U8(3).U8(4);

    Ww::FWwiseArchive Ar("t.bnk", b.Bytes);
    const auto source = Ww::WwiseDataSource::Archive();
    const Ww::WwiseReader reader(Ar, source);

    CHECK(reader.MidiData != nullptr);
    if (reader.MidiData != nullptr)
    {
        CHECK(reader.MidiData->LoadedSize() == 12);   // 8 header + 4 payload
        CHECK(reader.MidiData->GetData().size() == 12);
    }
}

static void TestRiffSectionSizeThrows()
{
    // A RIFF section claiming more bytes than the archive holds is C#'s signal that this is not a wem.
    Buf b;
    b.U32(static_cast<uint32_t>(Ww::Enums::EChunkID::RIFF)).I32(1000).Pad(4);

    Ww::FWwiseArchive Ar("t.wem", b.Bytes);
    const auto source = Ww::WwiseDataSource::Archive();

    bool threw = false;
    try { const Ww::WwiseReader reader(Ar, source); }
    catch (const Ww::RIFFSectionSizeException&) { threw = true; }
    CHECK(threw);
}

static void TestAkEntryOffsetIsMultiplied()
{
    // The offset is stored divided by OffsetMultiplier; the real byte offset is the product.
    Buf b;
    b.U32(0xABCD1234).U32(16).I32(64).U32(8).U32(2);

    Ww::FWwiseArchive Ar("t.pck", b.Bytes);
    Ww::Objects::AkEntry entry(Ar, /*isSoundBank*/ true);

    CHECK(entry.NameHash == 0xABCD1234u);
    CHECK(entry.OffsetMultiplier == 16u);
    CHECK(entry.Size == 64);
    CHECK(entry.Offset == 8u);
    CHECK(static_cast<int64_t>(entry.Offset) * entry.OffsetMultiplier == 128);
    CHECK(entry.FolderId == 2u);

    std::vector<Ww::Objects::AkFolder> folders;
    entry.ReadAudioPath(folders);          // no matching folder -> bare file name, as C#'s Path.Combine("")
    CHECK(entry.AudioPath == "2882343476.bnk");  // 0xABCD1234 rendered as an unsigned decimal
}

static void TestAkEntryExternalNameIsSixtyFourBit()
{
    // An external entry's name hash is 8 bytes, not 4 -- get this wrong and every later field shifts.
    Buf b;
    b.U64(0x1122334455667788ull).U32(1).I32(4).U32(0).U32(0);

    Ww::FWwiseArchive Ar("t.pck", b.Bytes);
    const Ww::Objects::AkEntry entry(Ar, /*isSoundBank*/ false, /*externalSound*/ true);

    CHECK(entry.ExternalNameHash == 0x1122334455667788ull);
    CHECK(entry.NameHash == 0u);
    CHECK(entry.Size == 4);
    CHECK(Ar.Position == 24);
}

static void TestPropertyUtilCoercion()
{
    namespace P = CUE4Parse::UE4::Assets::Objects::Properties;

    // A holder is anything with a Properties vector; FStructFallback is the one this slice uses.
    Obj::FStructFallback holder;

    Obj::FPropertyTag intTag;
    intTag.Name = CUE4Parse::UE4::Objects::UObject::FName("Count");
    intTag.Tag = std::make_unique<P::IntProperty>(7);
    holder.Properties.push_back(std::move(intTag));

    Obj::FPropertyTag boolTag;
    boolTag.Name = CUE4Parse::UE4::Objects::UObject::FName("bStreaming");
    boolTag.Tag = std::make_unique<P::BoolProperty>(true);
    holder.Properties.push_back(std::move(boolTag));

    CHECK(PU::GetOrDefault<int32_t>(holder, "Count") == 7);
    CHECK(PU::GetOrDefault<bool>(holder, "bStreaming") == true);

    // A missing property falls back rather than throwing.
    CHECK(PU::GetOrDefault<int32_t>(holder, "Missing", -1) == -1);

    // Case sensitivity is opt-in, matching C#'s StringComparison parameter.
    CHECK(PU::GetOrDefault<int32_t>(holder, "count", -1) == -1);
    CHECK(PU::GetOrDefault<int32_t>(holder, "count", -1, /*ignoreCase*/ true) == 7);

    // Widening within the same signedness is allowed; crossing signedness is not.
    CHECK(PU::GetOrDefault<int64_t>(holder, "Count") == 7);
    CHECK(PU::GetOrDefault<uint32_t>(holder, "Count", 99u) == 99u);

    // Get<T> throws where GetOrDefault falls back.
    bool threw = false;
    try { (void) PU::Get<int32_t>(holder, "Missing"); } catch (...) { threw = true; }
    CHECK(threw);
}

static void TestPropertyUtilEnumFromInteger()
{
    namespace P = CUE4Parse::UE4::Assets::Objects::Properties;
    namespace EW = CUE4Parse::UE4::Assets::Exports::Wwise;

    Obj::FStructFallback holder;
    Obj::FPropertyTag tag;
    tag.Name = CUE4Parse::UE4::Objects::UObject::FName("PackagingStrategy");
    // Cooked, unversioned assets store a small enum as a byte; the enum arm converts through the
    // underlying type rather than needing the member's name.
    tag.Tag = std::make_unique<P::ByteProperty>(static_cast<uint8_t>(EW::EWwisePackagingStrategy::BulkData));
    holder.Properties.push_back(std::move(tag));

    CHECK(PU::GetOrDefault<EW::EWwisePackagingStrategy>(holder, "PackagingStrategy",
                                                        EW::EWwisePackagingStrategy::Source)
          == EW::EWwisePackagingStrategy::BulkData);

    // A missing enum property keeps the caller's default, which for FWwisePackagedFile is Source.
    CHECK(PU::GetOrDefault<EW::EWwisePackagingStrategy>(holder, "Absent", EW::EWwisePackagingStrategy::Source)
          == EW::EWwisePackagingStrategy::Source);
}

static void TestObjectDataResourceLayout()
{
    // The AddedCookedIndex version inserts a single byte after the flags; the older one does not.
    FakePackage pkg;
    Buf b;
    b.U32(1)          // Flags
     .U8(3)           // CookedIndex (AddedCookedIndex only)
     .U64(100).U64(200).U64(300).U64(400)
     .I32(0)          // FPackageIndex
     .U32(0x40);      // LegacyBulkDataFlags

    Rd::FByteArchive base("t.uasset", b.Bytes);
    Ar_::FAssetArchive Ar(base, &pkg);
    const CUE4Parse::UE4::Objects::UObject::FObjectDataResource r(
        Ar, CUE4Parse::UE4::Objects::UObject::EObjectDataResourceVersion::AddedCookedIndex);

    CHECK(r.CookedIndex.Value == 3);
    CHECK(r.CookedIndex.GetAsExtension() == "003");   // zero-padded to three digits, as C#'s "D3"
    CHECK(r.SerialOffset == 100);
    CHECK(r.RawSize == 400);
    CHECK(r.LegacyBulkDataFlags == 0x40u);
}

static void TestBlittedLayouts()
{
    // The bulk-data map entry is read as a blitted array, so its size is load-bearing.
    static_assert(sizeof(CUE4Parse::UE4::IO::Objects::FBulkDataMapEntry) == 32);
    static_assert(sizeof(CUE4Parse::UE4::IO::Objects::FBulkDataCookedIndex) == 1);
    CHECK(CUE4Parse::UE4::IO::Objects::FBulkDataCookedIndex::Default().IsDefault());
    CHECK(CUE4Parse::UE4::IO::Objects::FBulkDataCookedIndex::Default().GetAsExtension().empty());
}

int main()
{
    TestPayloadTypeExtensions();
    TestBulkDataFlagsHelpers();
    TestInlineHeaderWidths();
    TestOffsetFixUpAppliesBulkDataStart();
    TestSize64BitWidensBothCounts();
    TestBadDataVersionSkipsTwoBytesAndClearsFlag();
    TestInlinePayloadIsLazyAndSkipped();
    TestZeroSizeBulkDataReadsEmptyNotNull();
    TestPayloadAtEndOfFileSeeks();
    TestSeparateFilePayloadFailsWithoutRegistration();
    TestPayloadRegistryRoundTrip();
    TestPayloadMapIsSharedWithClones();
    TestDeferredByteDataKinds();
    TestReadDeferredByteDataFallsBackToCopy();
    TestTryReadSoundBankId();
    TestWwiseReaderSkipsUnknownSections();
    TestWwiseReaderMidiSectionRewindsEight();
    TestRiffSectionSizeThrows();
    TestAkEntryOffsetIsMultiplied();
    TestAkEntryExternalNameIsSixtyFourBit();
    TestPropertyUtilCoercion();
    TestPropertyUtilEnumFromInteger();
    TestObjectDataResourceLayout();
    TestBlittedLayouts();

    if (g_failures == 0) std::cout << "all bulk-data / Wwise-reader tests passed\n";
    return g_failures == 0 ? 0 : 1;
}
