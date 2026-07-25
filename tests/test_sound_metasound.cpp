// Tests the Sound + MetaSound export trees.
//
// As with the other export suites, including every header is itself part of the test: these are header-only,
// so a header nothing includes never compiles. The include list below is what makes the ported tree real and
// doubles as an inventory of it.
//
// Deserializing these types for real needs a full package/archive context, which is exercised elsewhere.
// What this suite pins is the part a mechanical C#-to-C++ translation gets wrong without a running game:
//   * the class hierarchy and which bases carry a virtual destructor (polymorphic ownership through them);
//   * the [Flags] helpers (EStreamedAudioChunk / ESoundWaveFlag) matching C#'s HasFlag;
//   * the [StructFallback] ctors reading an *empty* bag back as the documented defaults -- which also drives
//     every new PropertyUtil arm (scalar, named-struct, FPackageIndex, enum, nested StructFallback);
//   * the faithful quirks: FMetasoundFrontendClassMetadata reading Type off the enum's *name* (so it always
//     defaults), and FDialogueContext comparing unresolved indices as not-equal (null == null is false).
#include <cstdint>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

#include "UE4/Objects/Engine/FStripDataFlags.h"
#include "UE4/Objects/Engine/FSubtitleCue.h"
#include "UE4/Objects/UObject/FFormatContainer.h"

#include "UE4/Assets/Exports/Sound/FBaseAttenuationSettings.h"
#include "UE4/Assets/Exports/Sound/FSoundAttenuationSettings.h"
#include "UE4/Assets/Exports/Sound/FStreamedAudioChunk.h"
#include "UE4/Assets/Exports/Sound/FStreamedAudioPlatformData.h"
#include "UE4/Assets/Exports/Sound/USoundBase.h"
#include "UE4/Assets/Exports/Sound/USoundWave.h"
#include "UE4/Assets/Exports/Sound/USoundSourceBus.h"
#include "UE4/Assets/Exports/Sound/USoundWaveProcedural.h"
#include "UE4/Assets/Exports/Sound/USoundClass.h"
#include "UE4/Assets/Exports/Sound/USoundCue.h"
#include "UE4/Assets/Exports/Sound/UDialogueWave.h"
#include "UE4/Assets/Exports/Sound/UMetaSoundSource.h"

#include "UE4/Assets/Exports/Sound/Node/FDialogueContext.h"
#include "UE4/Assets/Exports/Sound/Node/FDialogueContextMapping.h"
#include "UE4/Assets/Exports/Sound/Node/FDialogueWaveParameter.h"
#include "UE4/Assets/Exports/Sound/Node/SoundNodeTypes.h"
#include "UE4/Assets/Exports/Sound/Node/USoundNode.h"
#include "UE4/Assets/Exports/Sound/Node/USoundNodeDialoguePlayer.h"
#include "UE4/Assets/Exports/Sound/Node/USoundNodeWavePlayer.h"

#include "UE4/Assets/Exports/MetaSound/FMetasoundFrontendClass.h"
#include "UE4/Assets/Exports/MetaSound/FMetasoundFrontendClassMetadata.h"
#include "UE4/Assets/Exports/MetaSound/FMetasoundFrontendDocument.h"
#include "UE4/Assets/Exports/MetaSound/FMetasoundFrontendVertex.h"
#include "UE4/Assets/Exports/MetaSound/UMetaSoundPatch.h"
#include "UE4/Assets/Exports/MetaSound/UMetaSoundSource.h"

using namespace CUE4Parse::UE4::Assets;
using namespace CUE4Parse::UE4::Assets::Exports;
using CUE4Parse::UE4::Assets::Objects::FStructFallback;
using CUE4Parse::UE4::Objects::Engine::FSubtitleCue;

static int g_failures = 0;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            std::cerr << "CHECK failed: " #cond " (" << __FILE__ << ':'    \
                      << __LINE__ << ")\n";                                \
            ++g_failures;                                                  \
        }                                                                  \
    } while (0)

// ---- compile-time shape of the tree ---------------------------------------------------------------

// The Sound class hierarchy, exactly as C# derives it.
static_assert(std::is_base_of_v<UObject, Sound::USoundBase>);
static_assert(std::is_base_of_v<Sound::USoundBase, Sound::USoundWave>);
static_assert(std::is_base_of_v<Sound::USoundWave, Sound::USoundWaveProcedural>);
static_assert(std::is_base_of_v<Sound::USoundWave, Sound::USoundSourceBus>);
static_assert(std::is_base_of_v<Sound::USoundBase, Sound::USoundCue>);
static_assert(std::is_base_of_v<Sound::USoundWaveProcedural, Sound::UMetaUSoundSource>);
static_assert(std::is_base_of_v<Sound::USoundWaveProcedural, MetaSound::UMetaSoundSource>);

// The sound-node graph.
static_assert(std::is_base_of_v<UObject, Sound::Node::USoundNode>);
static_assert(std::is_base_of_v<Sound::Node::USoundNode, Sound::Node::USoundNodeMixer>);
static_assert(std::is_base_of_v<Sound::Node::USoundNode, Sound::Node::USoundNodeRandom>);
static_assert(std::is_base_of_v<Sound::Node::USoundNode, Sound::Node::USoundNodeDialoguePlayer>);
static_assert(std::is_base_of_v<Sound::Node::USoundNodeAssetReferencer, Sound::Node::USoundNodeWavePlayer>);

// Attenuation carries a virtual dtor because it is a base (FSoundAttenuationSettings derives it).
static_assert(std::has_virtual_destructor_v<Sound::FBaseAttenuationSettings>);
static_assert(std::is_base_of_v<Sound::FBaseAttenuationSettings, Sound::FSoundAttenuationSettings>);

// The MetaSound frontend base structs are extended, so they too are polymorphic.
static_assert(std::has_virtual_destructor_v<MetaSound::FMetasoundFrontendVertex>);
static_assert(std::has_virtual_destructor_v<MetaSound::FMetasoundFrontendClass>);

// The [Flags] enums are 32-bit, matching the on-disk width C# reads.
static_assert(std::is_same_v<std::underlying_type_t<Sound::EStreamedAudioChunk>, uint32_t>);
static_assert(std::is_same_v<std::underlying_type_t<Sound::ESoundWaveFlag>, uint32_t>);

// ---- behaviour ------------------------------------------------------------------------------------

static void TestFlagHelpers()
{
    using Sound::EStreamedAudioChunk;
    const auto both = static_cast<EStreamedAudioChunk>(
        static_cast<uint32_t>(EStreamedAudioChunk::IsCooked) |
        static_cast<uint32_t>(EStreamedAudioChunk::HasSeekOffset));
    CHECK(Sound::HasFlag(both, EStreamedAudioChunk::IsCooked));
    CHECK(Sound::HasFlag(both, EStreamedAudioChunk::HasSeekOffset));
    CHECK(!Sound::HasFlag(both, EStreamedAudioChunk::IsInlined));
}

static void TestStructFallbackDefaults()
{
    // An empty bag: every GetOrDefault misses and returns its default, which is what the ctors must yield.
    const FStructFallback empty;

    Sound::FBaseAttenuationSettings att(empty);
    CHECK(att.FalloffDistance == 0.0f);

    Sound::FSoundAttenuationSettings satt(empty);
    CHECK(satt.FalloffDistance == 0.0f); // read through the base ctor

    FSubtitleCue cue(empty);
    CHECK(cue.Text.empty());
    CHECK(cue.Time == 0.0f);

    // FPackageIndex arm + nested StructFallback arm (Context is itself a [StructFallback] type).
    Sound::Node::FDialogueContextMapping mapping(empty);
    CHECK(mapping.SoundWave.IsNull());
    CHECK(mapping.Context.Speaker.IsNull());
    CHECK(mapping.Context.Targets.empty());

    // The enum arm plus the documented nameof() quirk: Type is read off the enum's own name, which no
    // property matches, so it stays External regardless of the bag.
    MetaSound::FMetasoundFrontendClassMetadata meta(empty);
    CHECK(meta.Type == MetaSound::EMetasoundFrontendClassType::External);
}

static void TestDialogueContextEquality()
{
    // C#'s `!left?.Equals(right) ?? true`: an index with no owner resolves to no name, and comparing two
    // such contexts is *not* equal (null == null is false here). Two default contexts must not match.
    Sound::Node::FDialogueContext a;
    Sound::Node::FDialogueContext b;
    CHECK(!(a == b));
    CHECK(a != b);

    // GetWaveFromContext over an empty mapping list finds nothing and returns a null index.
    Sound::UDialogueWave wave;
    CHECK(wave.GetWaveFromContext(a).IsNull());
}

static void TestSoundCueDefaults()
{
    // The member initialisers before any Deserialize: the multipliers only take their 0.75/1.0 defaults
    // once Deserialize runs GetOrDefault, so fresh they are the plain float zero.
    Sound::USoundCue cue;
    CHECK(cue.VolumeMultiplier == 0.0f);
    CHECK(cue.PitchMultiplier == 0.0f);
    CHECK(cue.FirstNode.IsNull());

    // USoundWave's streaming guess starts true (the version-option default C# begins from).
    Sound::USoundWave sw;
    CHECK(sw.bStreaming == true);
    CHECK(sw.Subtitles.empty());
}

int main()
{
    TestFlagHelpers();
    TestStructFallbackDefaults();
    TestDialogueContextEquality();
    TestSoundCueDefaults();

    if (g_failures == 0) std::cout << "test_sound_metasound: all checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
