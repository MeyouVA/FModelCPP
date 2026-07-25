// Tests the Wwise soundbank reader: FWwiseArchive, the bank object model, the plugin-parameter tree and
// the HIRC hierarchy dispatch.
//
// As with the enum suites, including every header at once is a real part of the test -- these are all
// header-only, so a header nothing includes is never compiled. The full include list below is what makes
// the ported tree real, and it doubles as an inventory.
//
// The behavioural half feeds hand-built byte buffers through the readers and pins the things a mechanical
// C#-to-C++ translation gets wrong:
//   * FWwiseArchive hides the inherited Read<T>() template unless it is pulled back into scope;
//   * ReadBool() is one byte and ReadBoolean() is four -- both appear in this tree, and mixing them up
//     silently shifts everything after;
//   * Read7BitEncodedIntBE accumulates most-significant-first, the opposite of FArchive's LE variant;
//   * the version branches, where a field widens, moves, or disappears between bank versions;
//   * WwisePlugin's contract that the archive always lands on the declared payload end, whatever the
//     handler did;
//   * Hierarchy's recovery path: a throwing parse rewinds and re-reads as HierarchyGeneric.
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

#include "UE4/Wwise/Enums/EChunkID.h"
#include "UE4/Wwise/WwiseArchive.h"
#include "UE4/Wwise/WwiseFnv.h"
#include "UE4/Wwise/WwisePlugin.h"
#include "UE4/Wwise/WwiseVersionInfo.h"

#include "UE4/Wwise/Objects/AkAcousticTexture.h"
#include "UE4/Wwise/Objects/AkAdvSettingsParams.h"
#include "UE4/Wwise/Objects/AkAuxParams.h"
#include "UE4/Wwise/Objects/AkBankHeader.h"
#include "UE4/Wwise/Objects/AkBankSourceData.h"
#include "UE4/Wwise/Objects/AkChannelConfig.h"
#include "UE4/Wwise/Objects/AkChildren.h"
#include "UE4/Wwise/Objects/AkClipAutomation.h"
#include "UE4/Wwise/Objects/AkConversionTable.h"
#include "UE4/Wwise/Objects/AkDecisionTree.h"
#include "UE4/Wwise/Objects/AkDiffuseReverberator.h"
#include "UE4/Wwise/Objects/AkDuckInfo.h"
#include "UE4/Wwise/Objects/AkFXParams.h"
#include "UE4/Wwise/Objects/AkFeedbackInfo.h"
#include "UE4/Wwise/Objects/AkFolder.h"
#include "UE4/Wwise/Objects/AkGameSync.h"
#include "UE4/Wwise/Objects/AkMediaMap.h"
#include "UE4/Wwise/Objects/AkMeterInfo.h"
#include "UE4/Wwise/Objects/AkMusicFade.h"
#include "UE4/Wwise/Objects/AkMusicMarkerWwise.h"
#include "UE4/Wwise/Objects/AkMusicRanSeqPlaylistItem.h"
#include "UE4/Wwise/Objects/AkMusicSwitchAssoc.h"
#include "UE4/Wwise/Objects/AkMusicTrackTransRule.h"
#include "UE4/Wwise/Objects/AkMusicTransitionRule.h"
#include "UE4/Wwise/Objects/AkPlayList.h"
#include "UE4/Wwise/Objects/AkPositioningParams.h"
#include "UE4/Wwise/Objects/AkPropBundle.h"
#include "UE4/Wwise/Objects/AkRTPC.h"
#include "UE4/Wwise/Objects/AkRTPCRamping.h"
#include "UE4/Wwise/Objects/AkStateChunk.h"
#include "UE4/Wwise/Objects/AkStateGroupInfo.h"
#include "UE4/Wwise/Objects/AkStatePropertyInfo.h"
#include "UE4/Wwise/Objects/AkStateTransition.h"
#include "UE4/Wwise/Objects/AkStinger.h"
#include "UE4/Wwise/Objects/AkSwitchGroup.h"
#include "UE4/Wwise/Objects/AkSwitchPackage.h"
#include "UE4/Wwise/Objects/AkSwitchParams.h"
#include "UE4/Wwise/Objects/AkTrackSrcInfo.h"
#include "UE4/Wwise/Objects/AkTrackSwitchParams.h"
#include "UE4/Wwise/Objects/CAkEnvironmentsMgr.h"
#include "UE4/Wwise/Objects/CAkLayer.h"
#include "UE4/Wwise/Objects/GlobalSettings.h"
#include "UE4/Wwise/Objects/ICAkIndexable.h"
#include "UE4/Wwise/Objects/MediaHeader.h"
#include "UE4/Wwise/Objects/Setting.h"

#include "UE4/Wwise/Objects/Actions/AkRandomizerModifier.h"
#include "UE4/Wwise/Objects/Actions/CAkActionBase.h"
#include "UE4/Wwise/Objects/Actions/CAkActionBypassFX.h"
#include "UE4/Wwise/Objects/Actions/CAkActionExcept.h"
#include "UE4/Wwise/Objects/Actions/CAkActionParams.h"
#include "UE4/Wwise/Objects/Actions/CAkActionPause.h"
#include "UE4/Wwise/Objects/Actions/CAkActionPlay.h"
#include "UE4/Wwise/Objects/Actions/CAkActionResume.h"
#include "UE4/Wwise/Objects/Actions/CAkActionSeek.h"
#include "UE4/Wwise/Objects/Actions/CAkActionSetAkProp.h"
#include "UE4/Wwise/Objects/Actions/CAkActionSetFX.h"
#include "UE4/Wwise/Objects/Actions/CAkActionSetGameParameter.h"
#include "UE4/Wwise/Objects/Actions/CAkActionSetState.h"
#include "UE4/Wwise/Objects/Actions/CAkActionSetSwitch.h"
#include "UE4/Wwise/Objects/Actions/CAkActionStop.h"

#include "UE4/Wwise/Objects/HIRC/AbstractHierarchy.h"
#include "UE4/Wwise/Objects/HIRC/BaseHierarchy.h"
#include "UE4/Wwise/Objects/HIRC/BaseHierarchyBus.h"
#include "UE4/Wwise/Objects/HIRC/BaseHierarchyFx.h"
#include "UE4/Wwise/Objects/HIRC/BaseHierarchyModulator.h"
#include "UE4/Wwise/Objects/HIRC/BaseHierarchyMusic.h"
#include "UE4/Wwise/Objects/HIRC/Hierarchy.h"
#include "UE4/Wwise/Objects/HIRC/HierarchyGeneric.h"
#include "UE4/Wwise/Objects/HIRC/Containers/HierarchyActorMixer.h"
#include "UE4/Wwise/Objects/HIRC/Containers/HierarchyAttenuation.h"
#include "UE4/Wwise/Objects/HIRC/Containers/HierarchyBusVariants.h"
#include "UE4/Wwise/Objects/HIRC/Containers/HierarchyDialogueEvent.h"
#include "UE4/Wwise/Objects/HIRC/Containers/HierarchyEffect.h"
#include "UE4/Wwise/Objects/HIRC/Containers/HierarchyEvent.h"
#include "UE4/Wwise/Objects/HIRC/Containers/HierarchyEventAction.h"
#include "UE4/Wwise/Objects/HIRC/Containers/HierarchyFxVariants.h"
#include "UE4/Wwise/Objects/HIRC/Containers/HierarchyLayerContainer.h"
#include "UE4/Wwise/Objects/HIRC/Containers/HierarchyModulatorVariants.h"
#include "UE4/Wwise/Objects/HIRC/Containers/HierarchyMusicRandomSequenceContainer.h"
#include "UE4/Wwise/Objects/HIRC/Containers/HierarchyMusicSegment.h"
#include "UE4/Wwise/Objects/HIRC/Containers/HierarchyMusicSwitchContainer.h"
#include "UE4/Wwise/Objects/HIRC/Containers/HierarchyMusicTrack.h"
#include "UE4/Wwise/Objects/HIRC/Containers/HierarchyRandomSequenceContainer.h"
#include "UE4/Wwise/Objects/HIRC/Containers/HierarchySettings.h"
#include "UE4/Wwise/Objects/HIRC/Containers/HierarchySidechainMix.h"
#include "UE4/Wwise/Objects/HIRC/Containers/HierarchySoundSfxVoice.h"
#include "UE4/Wwise/Objects/HIRC/Containers/HierarchySwitchContainer.h"

// The plugin tree. Several of these are only reachable through WwisePlugin's dispatch table, so listing
// them here is what compiles them.
#include "UE4/Wwise/Plugins/CAk3DAudioBedMixerFXParams.h"
#include "UE4/Wwise/Plugins/CAkAsioParams.h"
#include "UE4/Wwise/Plugins/CAkChannelRouterFXParams.h"
#include "UE4/Wwise/Plugins/CAkCompressorFXParams.h"
#include "UE4/Wwise/Plugins/CAkConvolutionReverbFXParams.h"
#include "UE4/Wwise/Plugins/CAkDefaultSinkParams.h"
#include "UE4/Wwise/Plugins/CAkDelayFXParams.h"
#include "UE4/Wwise/Plugins/CAkExpanderFXParams.h"
#include "UE4/Wwise/Plugins/CAkFDNReverbFXParams.h"
#include "UE4/Wwise/Plugins/CAkFlangerFXParams.h"
#include "UE4/Wwise/Plugins/CAkFxSrcAudioInputParams.h"
#include "UE4/Wwise/Plugins/CAkFxSrcSilenceParams.h"
#include "UE4/Wwise/Plugins/CAkFxSrcSineParams.h"
#include "UE4/Wwise/Plugins/CAkGainFXParams.h"
#include "UE4/Wwise/Plugins/CAkGranularSynthParams.h"
#include "UE4/Wwise/Plugins/CAkGuitarDistortionFXParams.h"
#include "UE4/Wwise/Plugins/CAkHarmonizerFXParams.h"
#include "UE4/Wwise/Plugins/CAkImpacterParams.h"
#include "UE4/Wwise/Plugins/CAkMeterFXParams.h"
#include "UE4/Wwise/Plugins/CAkModalSynthParams.h"
#include "UE4/Wwise/Plugins/CAkMotionGeneratorParams.h"
#include "UE4/Wwise/Plugins/CAkMotionSourceParams.h"
#include "UE4/Wwise/Plugins/CAkParameterEQFXParams.h"
#include "UE4/Wwise/Plugins/CAkPeakLimiterFXParams.h"
#include "UE4/Wwise/Plugins/CAkPitchShifterFXParams.h"
#include "UE4/Wwise/Plugins/CAkRecorderADMFXParams.h"
#include "UE4/Wwise/Plugins/CAkRecorderFXParams.h"
#include "UE4/Wwise/Plugins/CAkReflectFXParams.h"
#include "UE4/Wwise/Plugins/CAkRoomVerbFXParams.h"
#include "UE4/Wwise/Plugins/CAkSidechainFXParams.h"
#include "UE4/Wwise/Plugins/CAkSoundSeedWindParams.h"
#include "UE4/Wwise/Plugins/CAkSoundSeedWooshParams.h"
#include "UE4/Wwise/Plugins/CAkStereoDelayFXParams.h"
#include "UE4/Wwise/Plugins/CAkSynthOneParams.h"
#include "UE4/Wwise/Plugins/CAkSystemOutputParams.h"
#include "UE4/Wwise/Plugins/CAkTimeStretchFXParams.h"
#include "UE4/Wwise/Plugins/CAkToneGenParams.h"
#include "UE4/Wwise/Plugins/CAkTremoloFXParams.h"
#include "UE4/Wwise/Plugins/CMicrosoftHRTFSinkParams.h"
#include "UE4/Wwise/Plugins/IAkPluginParam.h"
#include "UE4/Wwise/Plugins/Auro/CAuroHPFXParams.h"
#include "UE4/Wwise/Plugins/Auro/CAuroPannerParams.h"
#include "UE4/Wwise/Plugins/Bitcrush/CBitcrushFXParams.h"
#include "UE4/Wwise/Plugins/CrankcaseAudioREVModelPlayer/CREVSourceModelPlayerParams.h"
#include "UE4/Wwise/Plugins/MasteringSuite/CMasteringSuiteFXParams.h"
#include "UE4/Wwise/Plugins/McDSP/CMcDSPFutzBoxFXParams.h"
#include "UE4/Wwise/Plugins/McDSP/CMcDSPLimiterFXParams.h"
#include "UE4/Wwise/Plugins/MetaXRAudio/OculusEndpointParams.h"
#include "UE4/Wwise/Plugins/Mindseye/MindseyePluginParams.h"
#include "UE4/Wwise/Plugins/OculusSpatializer/COculusSpatializerFXParams.h"
#include "UE4/Wwise/Plugins/PolyspectralMBC/CMBCRuntimeParams.h"
#include "UE4/Wwise/Plugins/ResonanceAudio/ResonanceAudioParams.h"
#include "UE4/Wwise/Plugins/atmoky/CatmokyEarsFXParams.h"
#include "UE4/Wwise/Plugins/iZotope/CiZHybridReverbFXParams.h"
#include "UE4/Wwise/Plugins/iZotope/CiZTrashBoxModelerFXParams.h"
#include "UE4/Wwise/Plugins/iZotope/CiZTrashDelayFXParams.h"
#include "UE4/Wwise/Plugins/iZotope/CiZTrashDistortionFXParams.h"
#include "UE4/Wwise/Plugins/iZotope/CiZTrashDynamicsFXParams.h"
#include "UE4/Wwise/Plugins/iZotope/CiZTrashFiltersFXParams.h"
#include "UE4/Wwise/Plugins/iZotope/CiZTrashMultibandDistortionFXParams.h"

using namespace CUE4Parse::UE4::Wwise;
namespace Obj = CUE4Parse::UE4::Wwise::Objects;
namespace Hirc = CUE4Parse::UE4::Wwise::Objects::HIRC;
namespace Plug = CUE4Parse::UE4::Wwise::Plugins;
namespace En = CUE4Parse::UE4::Wwise::Enums;
namespace Fl = CUE4Parse::UE4::Wwise::Enums::Flags;

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
    Buf& U16(uint16_t v) { return Raw(&v, 2); }
    Buf& U32(uint32_t v) { return Raw(&v, 4); }
    Buf& I32(int32_t v) { return Raw(&v, 4); }
    Buf& U64(uint64_t v) { return Raw(&v, 8); }
    Buf& F32(float v) { return Raw(&v, 4); }
    Buf& F64(double v) { return Raw(&v, 8); }
    Buf& Bool32(bool v) { return U32(v ? 1u : 0u); }   // FArchive::ReadBoolean reads four bytes
    Buf& Str(const char* s) { while (*s) U8(static_cast<uint8_t>(*s++)); return U8(0); }
    Buf& Pad(int n) { for (int i = 0; i < n; i++) U8(0); return *this; }

    Buf& Raw(const void* p, int n)
    {
        const auto* b = static_cast<const uint8_t*>(p);
        Bytes.insert(Bytes.end(), b, b + n);
        return *this;
    }
};

static FWwiseArchive Open(const Buf& buf, uint32_t version)
{
    FWwiseArchive Ar("test.bnk", buf.Bytes);
    Ar.Version = version;
    return Ar;
}

// ---------------------------------------------------------------- FWwiseArchive

static void TestArchivePrimitives()
{
    // Read<T>() must survive FWwiseArchive overriding the byte-source Read(buffer, offset, count).
    // Without `using FArchive::Read;` the name is hidden and `Ar.Read<uint32_t>()` does not even parse.
    {
        auto Ar = Open(Buf().U32(0xDEADBEEF).U16(0x1234).U8(0x7F), 145);
        CHECK(Ar.Read<uint32_t>() == 0xDEADBEEFu);
        CHECK(Ar.Read<uint16_t>() == 0x1234);
        CHECK(Ar.Read<uint8_t>() == 0x7F);
        CHECK(Ar.Position == 7);
    }

    // ReadBool is one byte; the inherited ReadBoolean is four. Both are used in this tree.
    {
        auto Ar = Open(Buf().U8(1).U8(0).Bool32(true), 145);
        CHECK(Ar.ReadBool() == true);
        CHECK(Ar.ReadBool() == false);
        CHECK(Ar.Position == 2);
        CHECK(Ar.ReadBoolean() == true);
        CHECK(Ar.Position == 6);
    }

    // NUL-terminated, no length prefix.
    {
        auto Ar = Open(Buf().Str("Windows").U8(0xAA), 145);
        CHECK(Ar.ReadStzString() == "Windows");
        CHECK(Ar.Position == 8);
        CHECK(Ar.Read<uint8_t>() == 0xAA);
    }

    // An empty stz string is a lone terminator, not an error.
    {
        auto Ar = Open(Buf().Str(""), 145);
        CHECK(Ar.ReadStzString().empty());
        CHECK(Ar.Position == 1);
    }
}

static void TestSevenBitBigEndian()
{
    // Single byte: no continuation bit, value is the low seven bits.
    {
        auto Ar = Open(Buf().U8(0x7F), 145);
        CHECK(Ar.Read7BitEncodedIntBE() == 0x7F);
    }

    // Two bytes. Big-endian accumulation: (0x01 << 7) | 0x00 == 128. The little-endian reading of the
    // same bytes would be 0x00 | (0x01 << 7) -- the same here, so use an asymmetric pair below too.
    {
        auto Ar = Open(Buf().U8(0x81).U8(0x00), 145);
        CHECK(Ar.Read7BitEncodedIntBE() == 128);
    }

    // 0x82 0x03 -> (2 << 7) | 3 == 259 big-endian. Little-endian would give 2 | (3 << 7) == 386, so this
    // pair actually distinguishes the two encodings.
    {
        auto Ar = Open(Buf().U8(0x82).U8(0x03), 145);
        CHECK(Ar.Read7BitEncodedIntBE() == 259);
    }

    // Three bytes: (1 << 14) | (2 << 7) | 3.
    {
        auto Ar = Open(Buf().U8(0x81).U8(0x82).U8(0x03), 145);
        CHECK(Ar.Read7BitEncodedIntBE() == (1 << 14) | (2 << 7) | 3);
    }
}

static void TestVersionInfo()
{
    CHECK(IsSupported(145));
    CHECK(IsSupported(65));   // oldest supported
    CHECK(IsSupported(174));  // newest supported
    CHECK(!IsSupported(144)); // a real Wwise version, but not on the supported list
    CHECK(!IsSupported(0));

    auto Ar = Open(Buf(), 145);
    CHECK(Ar.IsSupported());
    Ar.Version = 999;
    CHECK(!Ar.IsSupported());
}

static void TestFnvHash()
{
    // FNV-1 (multiply then xor), not FNV-1a. The offset basis alone is what an empty name hashes to.
    CHECK(WwiseFnv::GetHash("") == 2166136261u);

    // Hashing is case-insensitive because GetHash lowercases first.
    CHECK(WwiseFnv::GetHash("Play_Music") == WwiseFnv::GetHash("play_music"));
    CHECK(WwiseFnv::GetHash("PLAY_MUSIC") == WwiseFnv::GetHashLower("play_music"));
    CHECK(WwiseFnv::GetHash("Play_Music") != WwiseFnv::GetHash("Play_Sound"));

    // Spelling out one step confirms the multiply-then-xor order: a single 'a' (0x61).
    constexpr uint32_t expected = (2166136261u * 16777619u) ^ 0x61u;
    CHECK(WwiseFnv::GetHash("a") == expected);
}

// ---------------------------------------------------------------- bank header

static void TestBankHeader()
{
    // Modern layout (> 141): version, bank id, language, alt-values, project id, bank type, 16-byte hash.
    {
        Buf b;
        b.U32(145).U32(0x1111).U32(0x2222).U32(0x10).U32(0x3333).U32(0x1E).Pad(0x10);
        auto Ar = Open(b, 0);
        const int sectionLength = static_cast<int>(b.Bytes.size());
        Obj::AkBankHeader h(Ar, sectionLength);
        CHECK(h.Version == 145);
        CHECK(h.SoundBankId == 0x1111u);
        CHECK(h.LanguageId == 0x2222u);
        CHECK(h.AltValues == Fl::EAltValuesFlags::UAlignment);
        CHECK(h.ProjectId == 0x3333u);
        CHECK(h.SoundBankType == En::EAkBankTypeEnum::Event);
        CHECK(h.BankHash.size() == 16);
        CHECK(h.FeedbackInBank == false);
        // gapSize = length - 0x14 - 0x04 - 0x10 == 0, so nothing is skipped.
        CHECK(Ar.Position == sectionLength);
    }

    // <= 126 reads a feedback word instead of the alt-values flags, and no bank type or hash.
    {
        Buf b;
        b.U32(112).U32(1).U32(2).U32(1).U32(9);
        auto Ar = Open(b, 0);
        Obj::AkBankHeader h(Ar, 0x14);
        CHECK(h.Version == 112);
        CHECK(h.FeedbackInBank == true);
        CHECK(h.ProjectId == 9u);
        CHECK(h.AltValues == static_cast<Fl::EAltValuesFlags>(0));
    }

    // Only the low bit of that word means "feedback".
    {
        auto Ar = Open(Buf().U32(112).U32(1).U32(2).U32(2).U32(0), 0);
        Obj::AkBankHeader h(Ar, 0x14);
        CHECK(h.FeedbackInBank == false);
    }

    // A section longer than the fields is padding, and the header must skip exactly the excess.
    {
        Buf b;
        b.U32(140).U32(1).U32(2).U32(0).U32(7).Pad(12);
        auto Ar = Open(b, 0);
        Obj::AkBankHeader h(Ar, 0x14 + 12);
        CHECK(h.Version == 140);
        CHECK(Ar.Position == 0x14 + 12);
    }
}

static void TestAkpkHeader()
{
    // Endianness is a four-byte bool here, not the one-byte FWwiseArchive::ReadBool.
    Buf b;
    b.Bool32(true).U32(100).U32(200).U32(300).U32(400);
    auto Ar = Open(b, 145);
    Obj::FAKPKHeader h(Ar);
    CHECK(h.Endianness == true);
    CHECK(Ar.Position == 20);
    CHECK(Obj::FAKPKHeader::NamesOffset == 28);
    CHECK(h.BanksOffset() == 128);
    CHECK(h.WemsOffset() == 328);
    CHECK(h.ExternalWemsOffset() == 628);
}

// ---------------------------------------------------------------- object model

static void TestChannelConfig()
{
    // Packed word: low byte channel count, next nibble config type, top 20 bits the mask.
    const uint32_t packed = 6u | (1u << 8) | (0x3Fu << 12);
    auto Ar = Open(Buf().U32(packed), 145);
    Obj::AkChannelConfig cfg(Ar);
    CHECK(cfg.NumChannels == 6);
    CHECK(cfg.ConfigType == En::EAkChannelConfigType::Standard);
    CHECK(cfg.ChannelMask == 0x3Fu);

    // The ushort overload takes a bare mask: no channel count, type forced to Standard.
    Obj::AkChannelConfig legacy(static_cast<uint16_t>(0x3F));
    CHECK(legacy.NumChannels == 0);
    CHECK(legacy.ChannelMask == 0x3Fu);
    CHECK(legacy.ConfigType == En::EAkChannelConfigType::Standard);
}

static void TestPropBundle()
{
    // Ids first, then values -- not interleaved pairs. Two props then two ranges.
    Buf b;
    b.U8(2).U8(0x10).U8(0x11).U32(0x40000000).U32(7);           // props
    b.U8(1).U8(0x20).U32(1).U32(2);                              // one range (min, max)
    auto Ar = Open(b, 145);
    Obj::AkPropBundle bundle(Ar);

    CHECK(bundle.Props.size() == 2);
    CHECK(bundle.Props[0].Id == 0x10);
    CHECK(bundle.Props[1].Id == 0x11);
    // The union has no type tag; C# guesses float above 0x10000000 and that guess is carried over.
    CHECK(bundle.Props[0].Value.IsFloat());
    CHECK(bundle.Props[0].Value.f32() == 2.0f);
    CHECK(!bundle.Props[1].Value.IsFloat());
    CHECK(bundle.Props[1].Value.u32 == 7u);

    CHECK(bundle.PropRanges.size() == 1);
    CHECK(bundle.PropRanges[0].Id == 0x20);
    CHECK(bundle.PropRanges[0].Min.u32 == 1u);
    CHECK(bundle.PropRanges[0].Max.u32 == 2u);
    CHECK(Ar.Position == static_cast<int64_t>(b.Bytes.size()));
}

static void TestRtpcParamIdWidth()
{
    // The param id is a uint at <= 89, a byte at <= 113, and a 7-bit BE int above -- three different
    // widths for the same field, so getting it wrong desynchronises everything after.
    auto conversionTable = [](Buf& b) { b.U8(0).U16(0); }; // scaling byte + zero graph points

    {
        Buf b; b.U32(1).U32(0x2A).U32(5); conversionTable(b);
        auto Ar = Open(b, 89);
        Obj::AkRtpc r(Ar);
        CHECK(r.ParamId == 0x2Au);
        CHECK(Ar.Position == static_cast<int64_t>(b.Bytes.size()));
    }
    {
        Buf b; b.U32(1).U8(0).U8(0).U8(0x2A).U32(5); conversionTable(b);
        auto Ar = Open(b, 113);
        Obj::AkRtpc r(Ar);
        CHECK(r.ParamId == 0x2Au);
        CHECK(Ar.Position == static_cast<int64_t>(b.Bytes.size()));
    }
    {
        Buf b; b.U32(1).U8(0).U8(0).U8(0x82).U8(0x03).U32(5); conversionTable(b);
        auto Ar = Open(b, 145);
        Obj::AkRtpc r(Ar);
        CHECK(r.ParamId == 259u); // the 7-bit BE pair from TestSevenBitBigEndian
        CHECK(Ar.Position == static_cast<int64_t>(b.Bytes.size()));
    }
}

static void TestConversionTable()
{
    // Modern: a scaling byte and a ushort count. readScaling:false skips the byte entirely.
    {
        Buf b;
        b.U8(2).U16(2).F32(0).F32(1).U32(0).F32(1).F32(2).U32(4);
        auto Ar = Open(b, 145);
        Obj::CAkConversionTable t(Ar);
        CHECK(t.Scaling == En::EAkCurveScaling::dB);
        CHECK(t.Size == 2);
        CHECK(t.GraphPoints.size() == 2);
        CHECK(t.GraphPoints[1].To == 2.0f);
        CHECK(Ar.Position == static_cast<int64_t>(b.Bytes.size()));
    }
    {
        Buf b; b.U16(0);
        auto Ar = Open(b, 145);
        Obj::CAkConversionTable t(Ar, false);
        CHECK(t.Scaling == En::EAkCurveScaling::None);
        CHECK(t.Size == 0);
        CHECK(Ar.Position == 2);
    }
}

static void TestStateAwareChunkCounts()
{
    // AkStateChunk uses plain uint/ushort counts; AkStateAwareChunk uses 7-bit BE ones. Same concept,
    // different encodings, and the property block only exists past 145.
    {
        Buf b;
        b.U32(1).U32(0xAA).U8(0).U16(1).U32(0x11).U32(0x22);
        auto Ar = Open(b, 120);
        Obj::AkStateChunk chunk(Ar);
        CHECK(chunk.Groups.size() == 1);
        CHECK(chunk.Groups[0].Id == 0xAAu);
        CHECK(chunk.Groups[0].States.size() == 1);
        CHECK(chunk.Groups[0].States[0].StateInstanceId.has_value());
        CHECK(*chunk.Groups[0].States[0].StateInstanceId == 0x22u);
        CHECK(Ar.Position == static_cast<int64_t>(b.Bytes.size()));
    }
    {
        Buf b;
        b.U8(0);                       // zero state-property infos
        b.U8(1);                       // one group
        b.U32(0xBB).U8(0);             // group id, sync type
        b.U8(1);                       // one state
        b.U32(0x33).U16(1).U16(9).F32(0.5f); // state id, one property
        auto Ar = Open(b, 150);
        Obj::AkStateAwareChunk chunk(Ar);
        CHECK(chunk.Groups.size() == 1);
        CHECK(chunk.Groups[0].States.size() == 1);
        // Past 145 a state carries properties instead of an instance id.
        CHECK(!chunk.Groups[0].States[0].StateInstanceId.has_value());
        CHECK(chunk.Groups[0].States[0].Properties.size() == 1);
        CHECK(chunk.Groups[0].States[0].Properties[0].Value == 0.5f);
        CHECK(Ar.Position == static_cast<int64_t>(b.Bytes.size()));
    }
}

static void TestActionExceptCountWidth()
{
    // <= 122 reads four bytes and truncates to a byte; above that it is a 7-bit BE int.
    {
        Buf b; b.U32(0x00000102).U32(1).U8(1).U32(2).U8(0); // count byte-truncates to 2
        auto Ar = Open(b, 122);
        Obj::Actions::CAkActionExcept except(Ar);
        CHECK(except.ExceptionElements.size() == 2);
        CHECK(except.ExceptionElements[0].Id == 1u);
        CHECK(except.ExceptionElements[0].IsBus == true);
    }
    {
        Buf b; b.U8(1).U32(0xABCD).U8(0);
        auto Ar = Open(b, 145);
        Obj::Actions::CAkActionExcept except(Ar);
        CHECK(except.ExceptionElements.size() == 1);
        CHECK(except.ExceptionElements[0].Id == 0xABCDu);
        CHECK(Ar.Position == 6);
    }
}

static void TestAdvSettingsFlagsAreRebuilt()
{
    // Modern: one packed byte per field.
    {
        Buf b; b.U8(0x01).U8(0x02).U16(4).U8(0x01).U8(0x03);
        auto Ar = Open(b, 145);
        Obj::AkAdvSettingsParams p(Ar);
        CHECK(p.MaxNumInstance == 4);
        CHECK(Ar.Position == 6);
    }
    // <= 89: individual bools that C# folds back into the same flag word.
    {
        Buf b;
        b.U8(0)          // virtual queue behavior
         .U8(1).U8(0)    // killNewest, useVirtualBehavior
         .U16(7)         // max instances
         .U8(1)          // global limit
         .U8(0)          // below-threshold behavior
         .U8(1).U8(0)    // maxNumInst / vvoices override parent
         .U8(1).U8(0).U8(0).U8(1); // hdr envelope quartet (version > 72)
        auto Ar = Open(b, 89);
        Obj::AkAdvSettingsParams p(Ar);
        CHECK(p.MaxNumInstance == 7);
        CHECK(p.IsGlobalLimit == true);
        CHECK(HasFlag(p.AdvSettingsFlags, Fl::EAkAdvSettingsFlags::KillNewest));
        CHECK(!HasFlag(p.AdvSettingsFlags, Fl::EAkAdvSettingsFlags::UseVirtualBehavior));
        CHECK(HasFlag(p.AdvSettingsFlags, Fl::EAkAdvSettingsFlags::IsMaxNumInstOverrideParent));
        CHECK(HasFlag(p.HdrEnvelopeFlags, Fl::EAkHdrEnvelopeFlags::OverrideHdrEnvelope));
        CHECK(HasFlag(p.HdrEnvelopeFlags, Fl::EAkHdrEnvelopeFlags::EnableEnvelope));
        CHECK(!HasFlag(p.HdrEnvelopeFlags, Fl::EAkHdrEnvelopeFlags::NormalizeLoudness));
        CHECK(Ar.Position == static_cast<int64_t>(b.Bytes.size()));
    }
}

static void TestMusicMarkerNameEncoding()
{
    // <= 62 has no name at all; 63..136 a count-prefixed ASCII blob including its NUL; above, an stz.
    {
        Buf b; b.U32(1).F64(0.0);
        auto Ar = Open(b, 62);
        Obj::AkMusicMarkerWwise m(Ar);
        CHECK(!m.MarkerName.has_value());
    }
    {
        Buf b; b.U32(1).F64(0.0).I32(6).Str("Entry");
        auto Ar = Open(b, 136);
        Obj::AkMusicMarkerWwise m(Ar);
        CHECK(m.MarkerName.has_value());
        CHECK(*m.MarkerName == "Entry"); // the trailing NUL is trimmed
    }
    {
        Buf b; b.U32(1).F64(0.0).Str("Exit");
        auto Ar = Open(b, 145);
        Obj::AkMusicMarkerWwise m(Ar);
        CHECK(m.MarkerName.has_value());
        CHECK(*m.MarkerName == "Exit");
    }
}

static void TestFolderNameIsUtf16()
{
    // C# reads `char`, which is two bytes in .NET -- these container names are UTF-16, and reading them
    // as bytes would stop at the first name character.
    Buf b;
    b.U32(0).U32(7);                                    // AkFolder: offset, id
    const char* name = " Windows ";
    for (const char* p = name; *p; ++p) b.U16(static_cast<uint16_t>(*p));
    b.U16(0);
    auto Ar = Open(b, 145);
    Obj::AkFolder folder(Ar);
    CHECK(folder.Id == 7u);
    folder.PopulateName(Ar, 8); // names section starts right after the two header words
    CHECK(folder.Name.has_value());
    CHECK(*folder.Name == "Windows"); // C#'s Trim() strips the surrounding spaces
}

// ---------------------------------------------------------------- plugins

static void TestDbToLinear()
{
    // 0 dB is unity; -6 dB is roughly a half.
    CHECK(Plug::DbToLinear(0.0f) == 1.0f);
    const float half = Plug::DbToLinear(-6.0f);
    CHECK(half > 0.49f && half < 0.51f);
    const float twenty = Plug::DbToLinear(20.0f);
    CHECK(twenty > 9.99f && twenty < 10.01f);
}

static void TestCompressorVersionGate()
{
    // Below 172 the channel-link percentage, sidechain scope and sidechain id are absent -- and C#
    // short-circuits, so no byte is consumed for the bool either.
    {
        Buf b;
        b.F32(1).F32(2).F32(3).F32(4).F32(0.0f).U8(1).U8(0);
        auto Ar = Open(b, 145);
        Plug::AkCompressorFXParams p(Ar);
        CHECK(p.Threshold == 1.0f);
        CHECK(p.OutputLevel == 1.0f); // 0 dB
        CHECK(p.ProcessLFE == true);
        CHECK(p.ChannelLink == false);
        CHECK(p.SidechainGlobalScope == false);
        CHECK(p.SidechainId == 0u);
        CHECK(Ar.Position == static_cast<int64_t>(b.Bytes.size()));
    }
    {
        Buf b;
        b.F32(1).F32(2).F32(3).F32(4).F32(0.0f).F32(0.5f).U8(1).U8(1).U8(1).U32(0x99);
        auto Ar = Open(b, 172);
        Plug::AkCompressorFXParams p(Ar);
        CHECK(p.ChannelLinkPercentage == 0.5f);
        CHECK(p.SidechainGlobalScope == true);
        CHECK(p.SidechainId == 0x99u);
        CHECK(Ar.Position == static_cast<int64_t>(b.Bytes.size()));
    }
}

static void TestDelayInterleavesGroups()
{
    // The delay time belongs to NonRTPC but is read first; feedback and mix are percentages.
    Buf b;
    b.F32(0.25f).F32(50.0f).F32(30.0f).F32(0.0f).U8(1).U8(0);
    auto Ar = Open(b, 145);
    Plug::AkDelayFXParams p(Ar);
    CHECK(p.NonRTPC.fDelayTime == 0.25f);
    CHECK(p.RTPC.fFeedback == 0.5f);
    // 30.0f * 0.01f is not exactly 0.3f -- 0.01f is not representable, so this needs a tolerance where
    // the 50%/0.5f case above happened to land exactly.
    CHECK(p.RTPC.fWetDryMix > 0.29999f && p.RTPC.fWetDryMix < 0.30001f);
    CHECK(p.RTPC.fOutputLevel == 1.0f);
    CHECK(p.RTPC.bFeedbackEnabled == true);
    CHECK(p.NonRTPC.bProcessLFE == false);
    CHECK(Ar.Position == static_cast<int64_t>(b.Bytes.size()));
}

static void TestPluginDispatch()
{
    // A zero-size payload yields no params and consumes only the size word.
    {
        Buf b; b.U32(0);
        auto Ar = Open(b, 145);
        AkPlugin plugin(static_cast<uint32_t>(En::EAkPluginId::AkGainFX));
        auto params = WwisePlugin::TryParsePluginParams(Ar, plugin);
        CHECK(params == nullptr);
        CHECK(Ar.Position == 4);
    }

    // A known id parses into its own type and lands exactly on the payload end.
    {
        Buf b; b.U32(8).F32(1.5f).F32(2.5f);
        auto Ar = Open(b, 145);
        AkPlugin plugin(static_cast<uint32_t>(En::EAkPluginId::AkGainFX));
        auto params = WwisePlugin::TryParsePluginParams(Ar, plugin);
        CHECK(params != nullptr);
        auto* gain = dynamic_cast<Plug::CAkGainFXParams*>(params.get());
        CHECK(gain != nullptr);
        if (gain) CHECK(gain->Params.fFullbandGain == 1.5f);
        CHECK(Ar.Position == 12);
    }

    // An id with no handler falls back to CAkDefaultParams, which keeps the raw bytes.
    {
        Buf b; b.U32(4).U32(0x12345678);
        auto Ar = Open(b, 145);
        AkPlugin plugin(0x00010203u); // not in the dispatch table
        auto params = WwisePlugin::TryParsePluginParams(Ar, plugin);
        auto* def = dynamic_cast<Plug::CAkDefaultParams*>(params.get());
        CHECK(def != nullptr);
        if (def) CHECK(def->PluginData.size() == 4);
        CHECK(Ar.Position == 8);
    }

    // The contract that matters: whatever the handler read, the archive ends on the declared end. Here
    // the payload claims 32 bytes but the gain handler only consumes 8.
    {
        Buf b; b.U32(32).F32(1).F32(2).Pad(24);
        auto Ar = Open(b, 145);
        AkPlugin plugin(static_cast<uint32_t>(En::EAkPluginId::AkGainFX));
        auto params = WwisePlugin::TryParsePluginParams(Ar, plugin);
        CHECK(params != nullptr);
        CHECK(Ar.Position == 36);
    }

    // A plugin id whose raw value is negative-as-int32 is skipped unless `always` is set. Reading stops
    // before the size word in that case, so nothing is consumed at all.
    {
        Buf b; b.U32(8).F32(1).F32(2);
        auto Ar = Open(b, 145);
        AkPlugin plugin(0xF0000002u);
        CHECK(WwisePlugin::TryParsePluginParams(Ar, plugin, false) == nullptr);
        CHECK(Ar.Position == 0);
    }

    // GetPluginId maps both sentinels to the invalid plugin.
    {
        auto Ar = Open(Buf().U32(0).U32(0xFFFFFFFFu).U32(0x00640002u), 145);
        CHECK(!WwisePlugin::GetPluginId(Ar).IsValid());
        CHECK(!WwisePlugin::GetPluginId(Ar).IsValid());
        const AkPlugin real = WwisePlugin::GetPluginId(Ar);
        CHECK(real.IsValid());
        CHECK(real.PluginId() == En::EAkPluginId::AkFxSrcSineSource);
        // Type is the low nibble, company the next byte.
        CHECK(real.Type() == En::EAkPluginType::Source);
    }

    // An invalid plugin reports the default company and no type rather than decoding 0xFFFFFFFF.
    {
        const AkPlugin none = AkPlugin::None();
        CHECK(none.CompanyId() == En::AkCompanyID::Audiokinetic);
        CHECK(none.Type() == En::EAkPluginType::None);
    }
}

// ---------------------------------------------------------------- HIRC

static void TestHierarchyEventCountEncoding()
{
    // <= 122: a plain uint count. Above: 7-bit BE.
    {
        Buf b; b.U32(0x77).U32(2).U32(0xA).U32(0xB);
        auto Ar = Open(b, 122);
        Hirc::Containers::HierarchyEvent e(Ar);
        CHECK(e.Id == 0x77u);
        CHECK(e.EventActionIds.size() == 2);
        CHECK(e.EventActionIds[1] == 0xBu);
        CHECK(Ar.Position == static_cast<int64_t>(b.Bytes.size()));
    }
    {
        Buf b; b.U32(0x77).U8(2).U32(0xA).U32(0xB);
        auto Ar = Open(b, 145);
        Hirc::Containers::HierarchyEvent e(Ar);
        CHECK(e.EventActionIds.size() == 2);
        CHECK(Ar.Position == static_cast<int64_t>(b.Bytes.size()));
    }
    // > 154 prefixes a limit-scope byte, an instance limit and a cooldown before the action list.
    {
        Buf b; b.U32(0x77).U8(1).U16(4).F32(2.5f).U8(1).U32(0xC);
        auto Ar = Open(b, 172);
        Hirc::Containers::HierarchyEvent e(Ar);
        CHECK(e.LimitScope == 1);
        CHECK(e.InstanceLimit == 4);
        CHECK(e.CooldownTime == 2.5f);
        CHECK(e.EventActionIds.size() == 1);
        CHECK(Ar.Position == static_cast<int64_t>(b.Bytes.size()));
    }
}

static void TestHierarchySettings()
{
    // The parameter type widened from byte to ushort at 127; ids first, values after.
    {
        Buf b; b.U32(5).U8(2).U8(1).U8(2).F32(0.5f).F32(1.5f);
        auto Ar = Open(b, 126);
        Hirc::Containers::HierarchySettings s(Ar);
        CHECK(s.Id == 5u);
        CHECK(s.SettingsCount == 2);
        CHECK(s.Settings.size() == 2);
        CHECK(s.Settings[0].SettingValue == 0.5f);
        CHECK(s.Settings[1].SettingValue == 1.5f);
        CHECK(Ar.Position == static_cast<int64_t>(b.Bytes.size()));
    }
    {
        Buf b; b.U32(5).U16(1).U16(3).F32(2.5f);
        auto Ar = Open(b, 145);
        Hirc::Containers::HierarchySettings s(Ar);
        CHECK(s.SettingsCount == 1);
        CHECK(s.Settings[0].SettingValue == 2.5f);
        CHECK(Ar.Position == static_cast<int64_t>(b.Bytes.size()));
    }
}

static void TestHierarchyDispatchAndRecovery()
{
    // A well-formed State entry parses into HierarchySettings and lands on the declared end.
    {
        Buf payload; payload.U32(0x42).U16(0);      // id, zero settings
        Buf b;
        b.U8(static_cast<uint8_t>(En::EAKBKHircType::State));
        b.U32(static_cast<uint32_t>(payload.Bytes.size()));
        b.Raw(payload.Bytes.data(), static_cast<int>(payload.Bytes.size()));
        auto Ar = Open(b, 145);
        Hirc::Hierarchy h(Ar);
        CHECK(h.Type == En::EAKBKHircType::State);
        CHECK(h.Data != nullptr);
        CHECK(h.Data->Id == 0x42u);
        CHECK(Ar.Position == static_cast<int64_t>(b.Bytes.size()));
    }

    // An unknown type falls through to HierarchyGeneric, which reads only the id.
    {
        Buf b;
        b.U8(0x7E).U32(8).U32(0x99).U32(0);
        auto Ar = Open(b, 145);
        Hirc::Hierarchy h(Ar);
        CHECK(h.Data != nullptr);
        CHECK(h.Data->Id == 0x99u);
        // Generic read 4 of the 8 declared bytes; the fixup still lands on the end.
        CHECK(Ar.Position == 13);
    }

    // Recovery: a Settings payload claiming more settings than there are bytes throws inside the typed
    // parse, and Hierarchy rewinds and re-reads as generic rather than propagating.
    {
        Buf b;
        b.U8(static_cast<uint8_t>(En::EAKBKHircType::State));
        b.U32(6).U32(0xABCD).U16(0xFFFF); // 65535 settings, but the buffer ends here
        auto Ar = Open(b, 145);
        Hirc::Hierarchy h(Ar);
        CHECK(h.Data != nullptr);
        CHECK(h.Data->Id == 0xABCDu); // the generic fallback still recovered the id
        CHECK(Ar.Position == 11);
    }

    // The type byte is mapped, not cast: 0x17 is AudioDevice in v125 numbering and SidechainMix now.
    {
        Buf b;
        b.U8(0x17).U32(4).U32(1);
        auto Ar125 = Open(b, 125);
        Hirc::Hierarchy h125(Ar125);
        CHECK(h125.Type == En::EAKBKHircType::AudioDevice);

        auto Ar145 = Open(b, 145);
        Hirc::Hierarchy h145(Ar145);
        CHECK(h145.Type == En::EAKBKHircType::SidechainMix);
    }
}

static void TestEventActionPayloadSelection()
{
    // SetState carries a CAkActionSetState payload; the kind tag says which alternative is present.
    Buf b;
    b.U32(0x10)                       // id
     .U8(0).U8(static_cast<uint8_t>(En::EAkActionType::SetState))
     .U32(0x20)                       // referenced id
     .U8(0)                           // isBus
     .U8(0).U8(0)                     // empty prop bundle (props, ranges)
     .U32(0xAA).U32(0xBB);            // state group id, target state id
    auto Ar = Open(b, 145);
    Hirc::Containers::HierarchyEventAction a(Ar);
    CHECK(a.EventActionType == En::EAkActionType::SetState);
    CHECK(a.ActionKind == Hirc::Containers::HierarchyEventAction::EActionKind::SetState);
    const auto* setState = a.Get<Obj::Actions::CAkActionSetState>();
    CHECK(setState != nullptr);
    if (setState)
    {
        CHECK(setState->StateGroupId == 0xAAu);
        CHECK(setState->TargetStateId == 0xBBu);
    }
    // Asking for the wrong alternative yields null rather than reinterpreting.
    CHECK(a.Get<Obj::Actions::CAkActionPlay>() == nullptr);
    CHECK(Ar.Position == static_cast<int64_t>(b.Bytes.size()));
}

// ---------------------------------------------------------------- layout / typing

static void TestBlittedStructLayouts()
{
    // These are read with a single Read<T>() rather than field by field, so their size is load-bearing.
    static_assert(sizeof(Obj::MediaHeader) == 12);
    static_assert(sizeof(Plug::AkGainFXParams) == 8);
    static_assert(sizeof(Plug::AkFxSrcSilenceParams) == 12);
    static_assert(sizeof(Plug::AkFilterBand) == 17); // packed: 4-byte enum + 3 floats + 1 bool
    static_assert(sizeof(Plug::AkFilterParams) == 16);
    static_assert(sizeof(Plug::EQModuleParamsStatic) == 14); // packed: byte enum + byte + 3 floats
    static_assert(sizeof(Plug::EQModuleParamsDynamic) == 16);
    static_assert(sizeof(Plug::FGranularValue) == 36);
    static_assert(sizeof(Plug::FGranularModulatorParams) == 17);
    static_assert(sizeof(Plug::McDSP::McDSPLimiterFXParams) == 20);

    // Underlying types matter for the same reason -- these are read straight off the wire.
    static_assert(std::is_same_v<std::underlying_type_t<En::EAKBKHircType>, uint8_t>);
    static_assert(std::is_same_v<std::underlying_type_t<En::EChunkID>, uint32_t>);
    static_assert(std::is_same_v<std::underlying_type_t<Plug::AkFilterType>, uint8_t>);
    static_assert(std::is_same_v<std::underlying_type_t<Plug::AkFilterTypeOld>, uint32_t>);
    static_assert(std::is_same_v<std::underlying_type_t<Plug::McDSP::FutzSIMType>, int32_t>);
}

static void TestPolymorphicOwnership()
{
    // The HIRC payload and the plugin params are both owned through base pointers, so the bases need
    // virtual destructors or the derived members leak.
    static_assert(std::has_virtual_destructor_v<Hirc::AbstractHierarchy>);
    static_assert(std::has_virtual_destructor_v<Plug::IAkPluginParam>);
    static_assert(std::has_virtual_destructor_v<Obj::ICAkIndexable>);
    static_assert(std::is_base_of_v<Obj::ICAkIndexable, Hirc::AbstractHierarchy>);
    static_assert(std::is_base_of_v<Hirc::AbstractHierarchy, Hirc::Containers::HierarchyAudioBus>);
    static_assert(std::is_base_of_v<Hirc::BaseHierarchyBus, Hirc::Containers::HierarchyAuxiliaryBus>);
    static_assert(std::is_base_of_v<Hirc::BaseHierarchyFx, Hirc::Containers::HierarchyAudioDevice>);
    static_assert(std::is_base_of_v<Hirc::BaseHierarchyModulator, Hirc::Containers::HierarchyLFO>);
    static_assert(std::is_base_of_v<Hirc::BaseHierarchyMusic, Hirc::Containers::HierarchyMusicSegment>);
}

static void TestArchivePositionIsIndependentAfterClone()
{
    // Documented divergence from C#: the C# clone shares the inner archive and therefore the cursor,
    // because Position is a virtual property there. In this port Position is plain archive state, so a
    // clone starts where the original was and then moves independently.
    auto Ar = Open(Buf().U32(1).U32(2).U32(3), 145);
    Ar.Read<uint32_t>();
    auto clone = Ar.Clone();
    CHECK(clone->Position == 4);
    CHECK(clone->Read<uint32_t>() == 2u);
    CHECK(clone->Position == 8);
    CHECK(Ar.Position == 4);          // the original did not move
    CHECK(Ar.Read<uint32_t>() == 2u); // and re-reads the same word
    // The clone carries the bank version, as C#'s does.
    CHECK(static_cast<FWwiseArchive*>(clone.get())->Version == 145u);
}

int main()
{
    TestArchivePrimitives();
    TestSevenBitBigEndian();
    TestVersionInfo();
    TestFnvHash();
    TestBankHeader();
    TestAkpkHeader();
    TestChannelConfig();
    TestPropBundle();
    TestRtpcParamIdWidth();
    TestConversionTable();
    TestStateAwareChunkCounts();
    TestActionExceptCountWidth();
    TestAdvSettingsFlagsAreRebuilt();
    TestMusicMarkerNameEncoding();
    TestFolderNameIsUtf16();
    TestDbToLinear();
    TestCompressorVersionGate();
    TestDelayInterleavesGroups();
    TestPluginDispatch();
    TestHierarchyEventCountEncoding();
    TestHierarchySettings();
    TestHierarchyDispatchAndRecovery();
    TestEventActionPayloadSelection();
    TestBlittedStructLayouts();
    TestPolymorphicOwnership();
    TestArchivePositionIsIndependentAfterClone();

    if (g_failures == 0) std::cout << "test_wwise: all checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
