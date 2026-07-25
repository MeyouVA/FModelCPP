// Tests ObjectTypeRegistry: that every serialized class name maps to the concrete C++ export type it should,
// and that the names C# deliberately leaves out stay out.
//
// This is the seam where a reflection-free port can silently go wrong. C# derives the registered name from the
// type itself (strip a leading 'U'/'A'), so name and type can never disagree; here they are two independent
// strings-and-types written by hand, and a typo produces no compile error -- just a package that constructs a
// bare UObject and loses every typed field. So each case below builds through the registry and dynamic_casts
// the result to the type the name is supposed to mean.
#include <iostream>
#include <memory>
#include <string>
#include <typeinfo>

#include "UE4/Assets/ObjectTypeRegistry.h"

#include "UE4/Assets/Exports/UObject.h"
#include "UE4/Assets/Exports/UObjectRedirector.h"
#include "UE4/Assets/Exports/Engine/UCurveTable.h"
#include "UE4/Assets/Exports/Engine/UDataTable.h"
#include "UE4/Assets/Exports/Internationalization/UStringTable.h"
#include "UE4/Objects/Engine/UBlueprintGeneratedClass.h"
#include "UE4/Objects/Engine/UUserDefinedEnum.h"
#include "UE4/Objects/Engine/UUserDefinedStruct.h"
#include "UE4/Objects/Engine/Curves/UCurveFloat.h"
#include "UE4/Objects/Engine/Curves/UCurveLinearColor.h"
#include "UE4/Objects/Engine/Curves/UCurveVector.h"
#include "UE4/Objects/UObject/UEnum.h"
#include "UE4/Objects/UObject/UFunction.h"
#include "UE4/Objects/UObject/UScriptStruct.h"
#include "UE4/Objects/UObject/UStruct.h"

#include "UE4/Assets/Exports/Sound/UDialogueWave.h"
#include "UE4/Assets/Exports/Sound/UMetaSoundSource.h"
#include "UE4/Assets/Exports/Sound/USoundBase.h"
#include "UE4/Assets/Exports/Sound/USoundClass.h"
#include "UE4/Assets/Exports/Sound/USoundCue.h"
#include "UE4/Assets/Exports/Sound/USoundSourceBus.h"
#include "UE4/Assets/Exports/Sound/USoundWave.h"
#include "UE4/Assets/Exports/Sound/USoundWaveProcedural.h"
#include "UE4/Assets/Exports/Sound/Node/SoundNodeTypes.h"
#include "UE4/Assets/Exports/Sound/Node/USoundNode.h"
#include "UE4/Assets/Exports/Sound/Node/USoundNodeAssetReferencer.h"
#include "UE4/Assets/Exports/Sound/Node/USoundNodeDialoguePlayer.h"
#include "UE4/Assets/Exports/Sound/Node/USoundNodeWave.h"
#include "UE4/Assets/Exports/Sound/Node/USoundNodeWavePlayer.h"
#include "UE4/Assets/Exports/MetaSound/UMetaSoundPatch.h"
#include "UE4/Assets/Exports/MetaSound/UMetaSoundSource.h"
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

using CUE4Parse::UE4::Assets::ObjectTypeRegistry;
using CUE4Parse::UE4::Assets::Exports::UObject;

namespace Sound = CUE4Parse::UE4::Assets::Exports::Sound;
namespace Node = CUE4Parse::UE4::Assets::Exports::Sound::Node;
namespace MetaSound = CUE4Parse::UE4::Assets::Exports::MetaSound;
namespace Wwise = CUE4Parse::UE4::Assets::Exports::Wwise;
namespace FMod = CUE4Parse::UE4::Assets::Exports::FMod;
namespace Engine = CUE4Parse::UE4::Assets::Exports::Engine;
namespace ObjEngine = CUE4Parse::UE4::Objects::Engine;
namespace ObjUObject = CUE4Parse::UE4::Objects::UObject;

static int g_failures = 0;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            std::cerr << "CHECK failed: " #cond " (" << __FILE__ << ':'    \
                      << __LINE__ << ")\n";                                \
            ++g_failures;                                                  \
        }                                                                  \
    } while (0)

// Builds `serializedName` through the registry and asserts the result is exactly a T -- not a base of one.
// The exact-type check matters: most of these types differ from their base only by which fields exist, so a
// name pointing one level up the hierarchy would still dynamic_cast<Base*> successfully.
template <typename T>
static void ExpectBuilds(const std::string& serializedName)
{
    auto factory = ObjectTypeRegistry::Get(serializedName);
    if (!factory)
    {
        std::cerr << "CHECK failed: nothing registered for \"" << serializedName << "\"\n";
        ++g_failures;
        return;
    }
    std::unique_ptr<UObject> obj = factory();
    if (obj == nullptr)
    {
        std::cerr << "CHECK failed: factory for \"" << serializedName << "\" returned null\n";
        ++g_failures;
        return;
    }
    if (dynamic_cast<T*>(obj.get()) == nullptr || typeid(*obj) != typeid(T))
    {
        std::cerr << "CHECK failed: \"" << serializedName << "\" built " << typeid(*obj).name()
                  << ", expected " << typeid(T).name() << " (" << __FILE__ << ")\n";
        ++g_failures;
    }
}

static void ExpectUnregistered(const std::string& serializedName)
{
    if (ObjectTypeRegistry::Get(serializedName))
    {
        std::cerr << "CHECK failed: \"" << serializedName << "\" is registered but should not be\n";
        ++g_failures;
    }
}

static void TestEngineTypes()
{
    ExpectBuilds<Engine::UDataTable>("DataTable");
    ExpectBuilds<Engine::UCurveTable>("CurveTable");
    ExpectBuilds<ObjUObject::UScriptStruct>("ScriptStruct");
    ExpectBuilds<ObjUObject::UEnum>("Enum");
    ExpectBuilds<ObjUObject::UFunction>("Function");
    ExpectBuilds<CUE4Parse::UE4::Assets::Exports::Internationalization::UStringTable>("StringTable");
    ExpectBuilds<CUE4Parse::UE4::Assets::Exports::UObjectRedirector>("ObjectRedirector");
    ExpectBuilds<ObjEngine::UBlueprintGeneratedClass>("BlueprintGeneratedClass");
    ExpectBuilds<ObjEngine::UUserDefinedStruct>("UserDefinedStruct");
    ExpectBuilds<ObjEngine::UUserDefinedEnum>("UserDefinedEnum");
    ExpectBuilds<ObjEngine::Curves::UCurveFloat>("CurveFloat");
    ExpectBuilds<ObjEngine::Curves::UCurveVector>("CurveVector");
    ExpectBuilds<ObjEngine::Curves::UCurveLinearColor>("CurveLinearColor");
}

static void TestSoundTypes()
{
    ExpectBuilds<Sound::UDialogueWave>("DialogueWave");
    ExpectBuilds<Sound::USoundClass>("SoundClass");
    ExpectBuilds<Sound::USoundCue>("SoundCue");
    ExpectBuilds<Sound::USoundWave>("SoundWave");
    ExpectBuilds<Sound::USoundWaveProcedural>("SoundWaveProcedural");
    ExpectBuilds<Sound::USoundSourceBus>("SoundSourceBus");
    // The typo'd sibling of MetaSound::UMetaSoundSource, kept verbatim from the C# (see the registry).
    ExpectBuilds<Sound::UMetaUSoundSource>("MetaUSoundSource");

    ExpectBuilds<Node::USoundNode>("SoundNode");
    ExpectBuilds<Node::USoundNodeAssetReferencer>("SoundNodeAssetReferencer");
    ExpectBuilds<Node::USoundNodeDialoguePlayer>("SoundNodeDialoguePlayer");
    ExpectBuilds<Node::USoundNodeWave>("SoundNodeWave");
    ExpectBuilds<Node::USoundNodeWavePlayer>("SoundNodeWavePlayer");
    ExpectBuilds<Node::USoundNodeDoppler>("SoundNodeDoppler");
    ExpectBuilds<Node::USoundNodeAttenuation>("SoundNodeAttenuation");
    ExpectBuilds<Node::USoundNodeQualityLevel>("SoundNodeQualityLevel");
    ExpectBuilds<Node::USoundNodeEnveloper>("SoundNodeEnveloper");
    ExpectBuilds<Node::USoundNodeDelay>("SoundNodeDelay");
    ExpectBuilds<Node::USoundNodeMixer>("SoundNodeMixer");
    ExpectBuilds<Node::USoundNodeModulator>("SoundNodeModulator");
    ExpectBuilds<Node::USoundNodeModulatorContinuous>("SoundNodeModulatorContinuous");
    ExpectBuilds<Node::USoundNodeRandom>("SoundNodeRandom");
    ExpectBuilds<Node::USoundNodeDistanceCrossFade>("SoundNodeDistanceCrossFade");
    ExpectBuilds<Node::USoundNodeParamCrossFade>("SoundNodeParamCrossFade");
    ExpectBuilds<Node::USoundNodeSwitch>("SoundNodeSwitch");
    ExpectBuilds<Node::USoundNodeSoundClass>("SoundNodeSoundClass");
    ExpectBuilds<Node::USoundNodeLooping>("SoundNodeLooping");
    ExpectBuilds<Node::USoundNodeBranch>("SoundNodeBranch");

    ExpectBuilds<MetaSound::UMetaSoundPatch>("MetaSoundPatch");
    ExpectBuilds<MetaSound::UMetaSoundSource>("MetaSoundSource");
}

static void TestWwiseTypes()
{
    ExpectBuilds<Wwise::UAkAudioType>("AkAudioType");
    ExpectBuilds<Wwise::UAkAcousticTexture>("AkAcousticTexture");
    ExpectBuilds<Wwise::UAkAudioBank>("AkAudioBank");
    ExpectBuilds<Wwise::UAkAudioDeviceShareSet>("AkAudioDeviceShareSet");
    ExpectBuilds<Wwise::UAkAudioEvent>("AkAudioEvent");
    ExpectBuilds<Wwise::UAkAuxBus>("AkAuxBus");
    ExpectBuilds<Wwise::UAkEffectShareSet>("AkEffectShareSet");
    ExpectBuilds<Wwise::UAkGroupValue>("AkGroupValue");
    ExpectBuilds<Wwise::UAkStateValue>("AkStateValue");
    ExpectBuilds<Wwise::UAkSwitchValue>("AkSwitchValue");
    ExpectBuilds<Wwise::UAkInitBank>("AkInitBank");
    ExpectBuilds<Wwise::UAkRtpc>("AkRtpc");
    ExpectBuilds<Wwise::UAkTrigger>("AkTrigger");
    ExpectBuilds<Wwise::UAkAssetData>("AkAssetData");
    ExpectBuilds<Wwise::UAkAssetDataWithMedia>("AkAssetDataWithMedia");
    ExpectBuilds<Wwise::UAkAssetDataSwitchContainer>("AkAssetDataSwitchContainer");
    ExpectBuilds<Wwise::UAkAudioEventData>("AkAudioEventData");
    ExpectBuilds<Wwise::UAkInitBankAssetData>("AkInitBankAssetData");
    ExpectBuilds<Wwise::UAkAssetPlatformData>("AkAssetPlatformData");
    ExpectBuilds<Wwise::UAkMediaAsset>("AkMediaAsset");
    ExpectBuilds<Wwise::UAkMediaAssetData>("AkMediaAssetData");
    ExpectBuilds<Wwise::UAkExternalMediaAsset>("AkExternalMediaAsset");
    ExpectBuilds<Wwise::UAkLocalizedMediaAsset>("AkLocalizedMediaAsset");
    ExpectBuilds<Wwise::UWwiseAssetLibrary>("WwiseAssetLibrary");
    // The per-game renames: same shape as UAkAudioBank/UAkAudioEvent, different serialized class name.
    ExpectBuilds<Wwise::UWuiBank>("WuiBank");
    ExpectBuilds<Wwise::UWwiseBank>("WwiseBank");
    ExpectBuilds<Wwise::UWuiEvent>("WuiEvent");
    ExpectBuilds<Wwise::UWwiseEvent>("WwiseEvent");
}

static void TestFModTypes()
{
    ExpectBuilds<FMod::UFMODBank>("FMODBank");
    ExpectBuilds<FMod::UFMODBankLookup>("FMODBankLookup");
    ExpectBuilds<FMod::UFMODBus>("FMODBus");
    ExpectBuilds<FMod::UFMODEvent>("FMODEvent");
    ExpectBuilds<FMod::UFMODSnapshot>("FMODSnapshot");
    ExpectBuilds<FMod::UFMODSnapshotReverb>("FMODSnapshotReverb");
    ExpectBuilds<FMod::UFMODVCA>("FMODVCA");
}

static void TestExclusions()
{
    // USoundBase is `abstract` in C#, so reflection skips it -- a cooked package never names it as a class
    // anyway (it is only ever a base). Registering it would be a divergence, not a convenience.
    ExpectUnregistered("SoundBase");
    // [SkipObjectRegistration] in C#.
    ExpectUnregistered("Struct");
    ExpectUnregistered("Class");
    ExpectUnregistered("MaterialInterface");
    // Names with no ported type still fall through (ConstructObject then builds a bare UObject).
    ExpectUnregistered("Texture2D");
    ExpectUnregistered("");
}

static void TestBlueprintSuffixFallback()
{
    // C#'s GetClass retries with a trailing "_C" stripped, so a Blueprint subclass of a registered native
    // type resolves to that type. Nothing else is stripped, and the retry needs a real base to hit.
    auto bp = ObjectTypeRegistry::Get("SoundCue_C");
    CHECK(static_cast<bool>(bp));
    if (bp) CHECK(dynamic_cast<Sound::USoundCue*>(bp().get()) != nullptr);

    ExpectUnregistered("SoundCue_D");
    ExpectUnregistered("NotARealClass_C");
    // A bare "_C" is 2 chars: C# would slice it to "" and miss; the port's size() > 2 guard skips the retry.
    ExpectUnregistered("_C");
}

static void TestRegisterClassOverride()
{
    // RegisterClass replaces an existing entry (C# does `_classes[name] = type`). FModel relies on this to
    // register game-specific types over the engine defaults.
    ObjectTypeRegistry::Get("SoundCue"); // force the one-time engine registration first
    ObjectTypeRegistry::RegisterClass("SoundCue",
        [] { return std::unique_ptr<UObject>(new Sound::USoundWave()); });
    ExpectBuilds<Sound::USoundWave>("SoundCue");
    ObjectTypeRegistry::RegisterClass("SoundCue",
        [] { return std::unique_ptr<UObject>(new Sound::USoundCue()); });
    ExpectBuilds<Sound::USoundCue>("SoundCue");
}

int main()
{
    TestEngineTypes();
    TestSoundTypes();
    TestWwiseTypes();
    TestFModTypes();
    TestExclusions();
    TestBlueprintSuffixFallback();
    TestRegisterClassOverride();

    if (g_failures == 0) std::cout << "test_object_type_registry: all checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
