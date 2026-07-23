// Tests the bulk-ported Wwise and FMod enum families (UE4/Wwise/Enums, UE4/Wwise/Enums/Flags,
// UE4/FMod/Enums).
//
// All 67 headers are included at once, which is the broadest check this slice gets: generated headers only
// ever compile because something includes them, and pulling them in together also proves the three
// namespaces keep their identically-named members apart (EAdvSettingsFlags and EAkAdvSettingsFlags both
// declare KillNewest; several enums declare None).
//
// The behavioural half deliberately does NOT re-assert every member of every enum -- that would just be the
// generator's output typed twice, and would pass even if the generator mistranslated the C#. It pins the
// things a mechanical translation can get *wrong*:
//   - the [Flags] operator set, including the ~ operator on a narrow underlying type;
//   - values C# spells in a syntax C++ does not share: binary literals with '_' digit separators, and
//     shifts that must not overflow their underlying type;
//   - the deliberate duplicate values C# marks with `#pragma warning disable CA1069`;
//   - the two hand-written version-mapping helpers, whose whole reason for existing is that the two
//     layouts are NOT a straight cast of one another.
#include <cstdint>
#include <iostream>
#include <string>
#include <type_traits>

#include "UE4/Wwise/Enums/EAkActionScope.h"
#include "UE4/Wwise/Enums/EAkActionType.h"
#include "UE4/Wwise/Enums/EAkBankTypeEnum.h"
#include "UE4/Wwise/Enums/EAkBelowThresholdBehavior.h"
#include "UE4/Wwise/Enums/EAKBKHircType.h"
#include "UE4/Wwise/Enums/EAKBKSourceType.h"
#include "UE4/Wwise/Enums/EAkBuiltInParam.h"
#include "UE4/Wwise/Enums/EAkChannelConfig.h"
#include "UE4/Wwise/Enums/EAkChannelConfigType.h"
#include "UE4/Wwise/Enums/EAkClipAutomationType.h"
#include "UE4/Wwise/Enums/EAkCompanyID.h"
#include "UE4/Wwise/Enums/EAkContainerMode.h"
#include "UE4/Wwise/Enums/EAkCurveInterpolation.h"
#include "UE4/Wwise/Enums/EAkCurveScaling.h"
#include "UE4/Wwise/Enums/EAkDecisionTreeMode.h"
#include "UE4/Wwise/Enums/EAkEntryType.h"
#include "UE4/Wwise/Enums/EAkFilterBehavior.h"
#include "UE4/Wwise/Enums/EAkGameSyncType.h"
#include "UE4/Wwise/Enums/EAkGroupType.h"
#include "UE4/Wwise/Enums/EAkJumpToSelType.h"
#include "UE4/Wwise/Enums/EAkMusicTrackType.h"
#include "UE4/Wwise/Enums/EAkPathMode.h"
#include "UE4/Wwise/Enums/EAkPluginId.h"
#include "UE4/Wwise/Enums/EAkPluginType.h"
#include "UE4/Wwise/Enums/EAkPropID.h"
#include "UE4/Wwise/Enums/EAkRandomMode.h"
#include "UE4/Wwise/Enums/EAkRtpcAccum.h"
#include "UE4/Wwise/Enums/EAkSyncType.h"
#include "UE4/Wwise/Enums/EAkTransitionMode.h"
#include "UE4/Wwise/Enums/EAkTransitionRampingType.h"
#include "UE4/Wwise/Enums/EAkValueMeaning.h"
#include "UE4/Wwise/Enums/EAkVirtualQueueBehavior.h"
#include "UE4/Wwise/Enums/EChunkID.h"
#include "UE4/Wwise/Enums/EHierarchyParameterType.h"
#include "UE4/Wwise/Enums/EOnSwitchMode.h"
#include "UE4/Wwise/Enums/EPositioningType.h"

#include "UE4/Wwise/Enums/Flags/EAdvSettingsFlags.h"
#include "UE4/Wwise/Enums/Flags/EAkAdvSettingsFlags.h"
#include "UE4/Wwise/Enums/Flags/EAkHdrEnvelopeFlags.h"
#include "UE4/Wwise/Enums/Flags/EAltValuesFlags.h"
#include "UE4/Wwise/Enums/Flags/EAuxParamsFlags.h"
#include "UE4/Wwise/Enums/Flags/EBankSourceFlags.h"
#include "UE4/Wwise/Enums/Flags/EBitsPositioningFlags.h"
#include "UE4/Wwise/Enums/Flags/EHdrEnvelopeFlags.h"
#include "UE4/Wwise/Enums/Flags/EMidiBehaviorFlags.h"
#include "UE4/Wwise/Enums/Flags/EMusicFlags.h"
#include "UE4/Wwise/Enums/Flags/EPauseOptionsFlags.h"
#include "UE4/Wwise/Enums/Flags/EPlaylistFlags.h"
#include "UE4/Wwise/Enums/Flags/EPriorityMidiFlags.h"
#include "UE4/Wwise/Enums/Flags/ERandomSequenceFlags.h"
#include "UE4/Wwise/Enums/Flags/EResumeOptionsFlags.h"

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

using namespace CUE4Parse::UE4::Wwise::Enums;
using namespace CUE4Parse::UE4::Wwise::Enums::Flags;
namespace FModEnums = CUE4Parse::UE4::FMod::Enums;

static int g_failures = 0;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            std::cerr << "FAIL: " << #cond << " (line " << __LINE__ << ")\n"; \
            ++g_failures;                                                  \
        }                                                                  \
    } while (0)

template <typename E>
static constexpr auto Raw(E value) { return static_cast<std::underlying_type_t<E>>(value); }

// ---------------------------------------------------------------- underlying types

// C# `: byte` / `: uint` / `: ushort` must survive as the C++ underlying type, because these enums are read
// straight off the wire: a widened underlying type would consume the wrong number of bytes.
static void TestUnderlyingTypes()
{
    static_assert(std::is_same_v<std::underlying_type_t<EAkActionScope>, uint8_t>);
    static_assert(std::is_same_v<std::underlying_type_t<EAKBKHircType>, uint8_t>);
    static_assert(std::is_same_v<std::underlying_type_t<EAkChannelConfig>, uint32_t>);
    static_assert(std::is_same_v<std::underlying_type_t<AkCompanyID>, uint16_t>);
    static_assert(std::is_same_v<std::underlying_type_t<EChunkID>, uint32_t>);
    static_assert(std::is_same_v<std::underlying_type_t<FModEnums::ERIFFID>, int32_t>);
    static_assert(std::is_same_v<std::underlying_type_t<EBitsPositioningFlags>, uint8_t>);
}

// ---------------------------------------------------------------- literal syntaxes C++ does not share

// C# writes these as binary literals with '_' digit separators (0b0110_0000); C++'s separator is '\''.
// A generator that passed the '_' through would not compile, but one that *dropped* it silently would give
// 0b01100000 -- the same value here, yet the check is cheap insurance on the conversion path.
static void TestBinaryLiterals()
{
    CHECK(Raw(EBitsPositioningFlags::PannerTypeMask) == 0x0C);
    CHECK(Raw(EBitsPositioningFlags::PositionTypeMask) == 0x60);
}

// `1 << 7` on a byte-backed enum: in C# the shift is done in int and then narrowed, so it must not be
// treated as an overflow here either.
static void TestHighBitShifts()
{
    CHECK(Raw(EBankSourceFlags::HasSource) == 0x80);
    CHECK(Raw(EBitsPositioningFlags::Unknown3d_7) == 0x80);
}

// Four-character-code enums: these are the bytes literally matched against a chunk header, so a byte-order
// slip would break every bank read. "AKPK" little-endian is 0x4B504B41.
static void TestFourCC()
{
    CHECK(Raw(EChunkID::AKPK) == 0x4B504B41);
    CHECK(Raw(EChunkID::RIFF) == 0x46464952);
    CHECK(Raw(FModEnums::ERIFFID::CHUNKID_RIFF) == 0x46464952);
    // Both families spell RIFF, and they must agree.
    CHECK(static_cast<int32_t>(EChunkID::RIFF) == Raw(FModEnums::ERIFFID::CHUNKID_RIFF));
}

// ---------------------------------------------------------------- deliberate duplicate values

// C# suppresses CA1069 in these files because the duplicates are intentional aliases. C++ allows duplicate
// enumerator values silently, so the risk is the reverse: a generator that *renumbered* to avoid a clash
// would go unnoticed.
static void TestIntentionalAliases()
{
    CHECK(EAkCurveInterpolation::Exp3 == EAkCurveInterpolation::LastFadeCurve);
    CHECK(Raw(EAkCurveInterpolation::LastFadeCurve) == 0x8);
    // ...and the member after the alias pair still continues from the right place.
    CHECK(Raw(EAkCurveInterpolation::Constant) == 0x9);
}

// ---------------------------------------------------------------- implicit numbering

// C#'s "no initialiser means previous + 1" has to survive across an explicitly-numbered first member and
// across a jump partway through the enum.
static void TestImplicitNumbering()
{
    CHECK(Raw(EAkActionScope::None) == 0x0);
    CHECK(Raw(EAkActionScope::GameObject) == 1);
    CHECK(Raw(EAkActionScope::GlobalGameObject) == 5);
    // The ladder restarts at the explicit arms rather than continuing from 5.
    CHECK(Raw(EAkActionScope::AllExceptId) == 0x09);
    CHECK(Raw(EAkActionScope::Ducking) == 0x20);

    // EAKBKHircType starts at 0x01 and runs implicitly for 22 members before jumping to 0x80.
    CHECK(Raw(EAKBKHircType::State) == 0x01);
    CHECK(Raw(EAKBKHircType::AudioDevice) == 0x15);
    CHECK(Raw(EAKBKHircType::SidechainMix) == 0x17);
    CHECK(Raw(EAKBKHircType::FeedbackBus) == 0x80);
    CHECK(Raw(EAKBKHircType::FeedbackNode) == 0x81);
}

// ---------------------------------------------------------------- [Flags] operators

static void TestFlagOperators()
{
    auto flags = EBankSourceFlags::IsLanguageSpecific | EBankSourceFlags::HasSource;
    CHECK(Raw(flags) == 0x81);
    CHECK(HasFlag(flags, EBankSourceFlags::IsLanguageSpecific));
    CHECK(HasFlag(flags, EBankSourceFlags::HasSource));
    CHECK(!HasFlag(flags, EBankSourceFlags::Prefetch));

    // HasFlag on a multi-bit mask means "all of these bits", not "any" -- C#'s Enum.HasFlag semantics.
    CHECK(HasFlag(flags, EBankSourceFlags::IsLanguageSpecific | EBankSourceFlags::HasSource));
    CHECK(!HasFlag(flags, EBankSourceFlags::IsLanguageSpecific | EBankSourceFlags::Prefetch));

    // None is a subset of everything, including of itself.
    CHECK(HasFlag(flags, EBankSourceFlags::None));
    CHECK(HasFlag(EBankSourceFlags::None, EBankSourceFlags::None));

    flags |= EBankSourceFlags::Prefetch;
    CHECK(HasFlag(flags, EBankSourceFlags::Prefetch));
    flags &= ~EBankSourceFlags::Prefetch;
    CHECK(!HasFlag(flags, EBankSourceFlags::Prefetch));
    // Clearing one bit must not have disturbed the others.
    CHECK(Raw(flags) == 0x81);

    // ~ on a byte-backed enum must stay inside the byte: a naive implementation promotes to int and gives
    // 0xFFFFFF7E, which then compares unequal to the byte-truncated expectation.
    CHECK(Raw(~EBankSourceFlags::IsLanguageSpecific) == 0xFE);
    CHECK(Raw(~EBitsPositioningFlags::PannerTypeMask) == 0xF3);

    // A multi-bit mask read out of a positioning byte.
    auto pos = EBitsPositioningFlags::PannerTypeMask | EBitsPositioningFlags::HasListenerRelativeRouting;
    CHECK(Raw(pos) == 0x0E);
    CHECK(HasFlag(pos, EBitsPositioningFlags::PannerTypeMask));
    CHECK(!HasFlag(pos, EBitsPositioningFlags::PositionTypeMask));
}

// ---------------------------------------------------------------- hand-written version mappers

// EAKBKHircType and EAKBKHircType_v125 are NOT a straight cast of one another: v125 lists
// FeedbackBus/FeedbackNode inline at 0x10/0x11, while the current enum moved them to 0x80/0x81 and reused
// 0x10 for FxShareSet. Everything from FxShareSet onwards is therefore shifted by two between layouts.
static void TestHircTypeMapping()
{
    // The divergence itself -- if these ever became equal, the mapper would be pointless and something is
    // wrong with one of the two enums.
    CHECK(Raw(EAKBKHircType::FxShareSet) == 0x10);
    CHECK(Raw(EAKBKHircType_v125::FeedbackBus) == 0x10);
    CHECK(Raw(EAKBKHircType_v125::FxShareSet) == 0x12);

    // Above 125 the raw byte is the current enum, cast straight through.
    CHECK(MapToCurrent(0x10, 126) == EAKBKHircType::FxShareSet);
    CHECK(MapToCurrent(0x01, 126) == EAKBKHircType::State);

    // At or below 125 the same byte means something else and must be translated.
    CHECK(MapToCurrent(0x10, 125) == EAKBKHircType::FeedbackBus);
    CHECK(MapToCurrent(0x12, 125) == EAKBKHircType::FxShareSet);
    // v125's Settings is the current State.
    CHECK(MapToCurrent(0x01, 125) == EAKBKHircType::State);

    // v125 runs out at 0x17 (AudioDevice, 23 members from 0x01) while the current enum reads the same byte
    // as SidechainMix -- the clearest single illustration of why the mapper is not a cast.
    CHECK(MapToCurrent(0x17, 125) == EAKBKHircType::AudioDevice);
    CHECK(MapToCurrent(0x17, 126) == EAKBKHircType::SidechainMix);

    // A byte past the end of v125 has no counterpart and yields the C# `_ => 0` sentinel, which is not a
    // declared member. 0x80 is FeedbackBus in the current enum, so a straight cast would have been wrong.
    CHECK(Raw(MapToCurrent(0x80, 125)) == 0);
    CHECK(MapToCurrent(0x80, 126) == EAKBKHircType::FeedbackBus);
}

static void TestHircTypeVersionString()
{
    // Above 125 the current member name.
    CHECK(std::string(ToVersionString(EAKBKHircType::State, 126)) == "State");
    // At or below 125 the *v125* name for the same member -- State was called Settings back then.
    CHECK(std::string(ToVersionString(EAKBKHircType::State, 125)) == "Settings");
    // A member both layouts agree on renders the same either way.
    CHECK(std::string(ToVersionString(EAKBKHircType::MusicTrack, 125)) == "MusicTrack");
    CHECK(std::string(ToVersionString(EAKBKHircType::MusicTrack, 126)) == "MusicTrack");
    // A member v125 never had falls through to the current name (C#'s `_ => type.ToString()`).
    CHECK(std::string(ToVersionString(EAKBKHircType::SidechainMix, 125)) == "SidechainMix");

    // NameOf reports null for an undeclared value rather than inventing a name.
    CHECK(NameOf(static_cast<EAKBKHircType>(0x7F)) == nullptr);
}

// Below bank version 150 the same action byte meant a different action; C# reinterprets the numeric value
// through the legacy enum rather than translating it.
static void TestActionTypeVersionString()
{
    CHECK(Raw(EAkActionType::SetBypassEffectSlot) == 0x33);
    CHECK(Raw(EEventActionType_v72_to_v150::BypassEffect) == 0x33);
    CHECK(std::string(ToVersionString(EAkActionType::SetBypassEffectSlot, 150)) == "SetBypassEffectSlot");
    CHECK(std::string(ToVersionString(EAkActionType::SetBypassEffectSlot, 149)) == "BypassEffect");
    // 150 is the boundary and belongs to the modern side (C#'s arm is `< 150`).
    CHECK(std::string(ToVersionString(EAkActionType::Stop, 150)) == "Stop");
    CHECK(NameOf(static_cast<EAkActionType>(0xFE)) == nullptr);
}

// C#'s BankSourceFlagsExtensions.MapToCurrent re-maps rather than casts: v112 puts HasSource at bit 1, the
// current enum at bit 7.
static void TestBankSourceFlagsMapping()
{
    CHECK(Raw(EBankSourceFlags_v112::HasSource) == 0x02);
    CHECK(Raw(EBankSourceFlags::HasSource) == 0x80);

    CHECK(MapToCurrent(EBankSourceFlags_v112::None) == EBankSourceFlags::None);
    CHECK(MapToCurrent(EBankSourceFlags_v112::HasSource) == EBankSourceFlags::HasSource);
    CHECK(MapToCurrent(EBankSourceFlags_v112::IsLanguageSpecific) == EBankSourceFlags::IsLanguageSpecific);

    const auto all = EBankSourceFlags_v112::IsLanguageSpecific | EBankSourceFlags_v112::HasSource |
                     EBankSourceFlags_v112::ExternallySupplied;
    const auto mapped = MapToCurrent(all);
    CHECK(HasFlag(mapped, EBankSourceFlags::IsLanguageSpecific));
    CHECK(HasFlag(mapped, EBankSourceFlags::HasSource));
    CHECK(HasFlag(mapped, EBankSourceFlags::ExternallySupplied));
    // Prefetch and NonCachable have no v112 counterpart and C# never sets them, so they stay clear even
    // when every legacy bit is on.
    CHECK(!HasFlag(mapped, EBankSourceFlags::Prefetch));
    CHECK(!HasFlag(mapped, EBankSourceFlags::NonCachable));
    CHECK(Raw(mapped) == 0x85);
}

// ---------------------------------------------------------------- namespace separation

// EAdvSettingsFlags and EAkAdvSettingsFlags are two different enums in the same namespace that share member
// names; several enums across the three namespaces declare None. Scoped enums are what keep these apart --
// this compiles only because none of them leaked their members.
static void TestNamesDoNotCollide()
{
    CHECK(Raw(EAdvSettingsFlags::KillNewest) != 0xFF);
    CHECK(Raw(EAkAdvSettingsFlags::KillNewest) != 0xFF);
    CHECK(Raw(EAkActionScope::None) == 0);
    CHECK(Raw(EBankSourceFlags::None) == 0);
    CHECK(Raw(FModEnums::EDSPType::FMOD_DSP_TYPE_UNKNOWN) == 0);
}

// ---------------------------------------------------------------- FMod side

static void TestFModEnums()
{
    // A four-CC list id, and the version enum's range.
    CHECK(Raw(FModEnums::ERIFFID::LISTID_PROJECT) == 0x4a4f5250);
    // The version enum does not start at zero -- it starts at the oldest file version FMOD still reads.
    CHECK(Raw(FModEnums::EFModVersion::OLDEST_SUPPORTED_FILEVERSION) == 0x2c);
    CHECK(Raw(FModEnums::EFModVersion::FILEVERSION_45) == 0x2d);
    // FILEVERSION_46 does not exist: 0x2e is skipped and 47 follows 45 directly.
    CHECK(Raw(FModEnums::EFModVersion::FILEVERSION_47) == 0x2f);
    CHECK(Raw(FModEnums::EDSPType::FMOD_DSP_TYPE_MIXER) == 0x1);
    CHECK(Raw(FModEnums::EPlaylistPlayMode::PlaylistPlayMode_PlaySequential) == 0);
    CHECK(Raw(FModEnums::EPlaylistPlayMode::PlaylistPlayMode_Max) == 0x6);
}

int main()
{
    TestUnderlyingTypes();
    TestBinaryLiterals();
    TestHighBitShifts();
    TestFourCC();
    TestIntentionalAliases();
    TestImplicitNumbering();
    TestFlagOperators();
    TestHircTypeMapping();
    TestHircTypeVersionString();
    TestActionTypeVersionString();
    TestBankSourceFlagsMapping();
    TestNamesDoNotCollide();
    TestFModEnums();

    if (g_failures == 0) std::cout << "test_wwise_fmod_enums: all checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
