// Ported from CUE4Parse/UE4/Assets/ObjectTypeRegistry.cs (manual, reflection-free registration).
#include "ObjectTypeRegistry.h"

#include <map>
#include <mutex>

#include "Exports/Engine/UDataTable.h"
#include "Exports/Engine/UCurveTable.h"
#include "Exports/Internationalization/UStringTable.h"
#include "Exports/UObjectRedirector.h"
#include "../Objects/UObject/UScriptStruct.h"
#include "../Objects/UObject/UEnum.h"
#include "../Objects/UObject/UFunction.h"
#include "../Objects/Engine/UBlueprintGeneratedClass.h"
#include "../Objects/Engine/UUserDefinedStruct.h"
#include "../Objects/Engine/UUserDefinedEnum.h"
#include "../Objects/Engine/Curves/UCurveFloat.h"
#include "../Objects/Engine/Curves/UCurveVector.h"
#include "../Objects/Engine/Curves/UCurveLinearColor.h"
// Sound
#include "Exports/Sound/UDialogueWave.h"
#include "Exports/Sound/UMetaSoundSource.h" // UMetaUSoundSource -- see the note at the registration below
#include "Exports/Sound/USoundClass.h"
#include "Exports/Sound/USoundCue.h"
#include "Exports/Sound/USoundSourceBus.h"
#include "Exports/Sound/USoundWave.h"
#include "Exports/Sound/USoundWaveProcedural.h"
#include "Exports/Sound/Node/USoundNode.h"
#include "Exports/Sound/Node/USoundNodeAssetReferencer.h"
#include "Exports/Sound/Node/USoundNodeDialoguePlayer.h"
#include "Exports/Sound/Node/USoundNodeWave.h"
#include "Exports/Sound/Node/USoundNodeWavePlayer.h"
#include "Exports/Sound/Node/SoundNodeTypes.h"
// MetaSound
#include "Exports/MetaSound/UMetaSoundPatch.h"
#include "Exports/MetaSound/UMetaSoundSource.h"
// Wwise
#include "Exports/Wwise/UAkAcousticTexture.h"
#include "Exports/Wwise/UAkAssetData.h"
#include "Exports/Wwise/UAkAssetDataSwitchContainer.h"
#include "Exports/Wwise/UAkAssetDataWithMedia.h"
#include "Exports/Wwise/UAkAssetPlatformData.h"
#include "Exports/Wwise/UAkAudioBank.h"
#include "Exports/Wwise/UAkAudioDeviceShareSet.h"
#include "Exports/Wwise/UAkAudioEvent.h"
#include "Exports/Wwise/UAkAudioEventData.h"
#include "Exports/Wwise/UAkAudioType.h"
#include "Exports/Wwise/UAkAuxBus.h"
#include "Exports/Wwise/UAkEffectShareSet.h"
#include "Exports/Wwise/UAkExternalMediaAsset.h"
#include "Exports/Wwise/UAkGroupValue.h"
#include "Exports/Wwise/UAkInitBank.h"
#include "Exports/Wwise/UAkInitBankAssetData.h"
#include "Exports/Wwise/UAkLocalizedMediaAsset.h"
#include "Exports/Wwise/UAkMediaAsset.h"
#include "Exports/Wwise/UAkMediaAssetData.h"
#include "Exports/Wwise/UAkRtpc.h"
#include "Exports/Wwise/UAkStateValue.h"
#include "Exports/Wwise/UAkSwitchValue.h"
#include "Exports/Wwise/UAkTrigger.h"
#include "Exports/Wwise/UWwiseAssetLibrary.h"
// Texture
#include "Exports/Texture/UCurveLinearColorAtlas.h"
#include "Exports/Texture/ULightMapTexture2D.h"
#include "Exports/Texture/ULightMapVirtualTexture2D.h"
#include "Exports/Texture/UMediaTexture.h"
#include "Exports/Texture/UPaperSprite.h"
#include "Exports/Texture/URuntimeVirtualTextureStreamingProxy.h"
#include "Exports/Texture/UShadowMapTexture2D.h"
#include "Exports/Texture/USubstanceAirTexture2D.h"
#include "Exports/Texture/UTerrainWeightMapTexture.h"
#include "Exports/Texture/UTexture.h"
#include "Exports/Texture/UTexture2D.h"
#include "Exports/Texture/UTexture2DArray.h"
#include "Exports/Texture/UTextureAllMipDataProviderFactory.h"
#include "Exports/Texture/UTextureCube.h"
#include "Exports/Texture/UTextureFlipBook.h"
#include "Exports/Texture/UTextureLightProfile.h"
#include "Exports/Texture/UTextureMipDataProviderFactory.h"
#include "Exports/Texture/UTextureMovie.h"
#include "Exports/Texture/UTextureProFX.h"
#include "Exports/Texture/UTextureRenderTarget.h"
#include "Exports/Texture/UTextureRenderTarget2D.h"
#include "Exports/Texture/UTextureRenderTargetCube.h"
#include "Exports/Texture/UVirtualTexture2D.h"
#include "Exports/Texture/UVolumeTexture.h"
#include "../Objects/Engine/UAssetUserData.h"
// FMOD
#include "Exports/FMod/UFMODBank.h"
#include "Exports/FMod/UFMODBankLookup.h"
#include "Exports/FMod/UFMODBus.h"
#include "Exports/FMod/UFMODEvent.h"
#include "Exports/FMod/UFMODSnapshot.h"
#include "Exports/FMod/UFMODSnapshotReverb.h"
#include "Exports/FMod/UFMODVCA.h"

namespace CUE4Parse::UE4::Assets
{
    namespace
    {
        // Meyers-singleton map (avoids static-init-order issues across translation units).
        std::map<std::string, ObjectTypeRegistry::Factory>& Registry()
        {
            static std::map<std::string, ObjectTypeRegistry::Factory> registry;
            return registry;
        }

        // Stands in for C#'s `RegisterClass(Type)`: there the serialized name is derived from the type by
        // stripping a leading 'U'/'A', here the caller passes that stripped name and the type as T.
        template <typename T>
        void Register(const std::string& serializedName)
        {
            ObjectTypeRegistry::RegisterClass(serializedName,
                [] { return std::unique_ptr<Exports::UObject>(new T()); });
        }

        // C#'s static ctor calls RegisterEngine(assembly), reflecting over every UObject subclass. Without
        // reflection we enumerate the ported engine export types by hand here. TODO: add types as ported.
        //
        // A C# type is registered unless it is abstract, an interface, or carries [SkipObjectRegistration];
        // those three exclusions are the only reason a ported concrete type would be missing below.
        void RegisterEngineTypes()
        {
            namespace Obj = CUE4Parse::UE4::Objects;

            Register<Exports::Engine::UDataTable>("DataTable");
            Register<Exports::Engine::UCurveTable>("CurveTable");
            // FProperties reflection types (C# registers these; UStruct/UClass are [SkipObjectRegistration]).
            Register<Obj::UObject::UScriptStruct>("ScriptStruct");
            Register<Obj::UObject::UEnum>("Enum");
            Register<Obj::UObject::UFunction>("Function");
            Register<Exports::Internationalization::UStringTable>("StringTable");
            Register<Exports::UObjectRedirector>("ObjectRedirector");
            // A cooked Blueprint class export (its class import is "BlueprintGeneratedClass"). Unlike UStruct/
            // UClass this concrete engine class IS registered in C#. Instances of a Blueprint (class name
            // "SomeBP_C") still fall through to a base UObject — resolving those needs the SuperStruct-chain
            // walk, which stays deferred (see Package::ConstructObject).
            Register<Obj::Engine::UBlueprintGeneratedClass>("BlueprintGeneratedClass");
            // Blueprint-authored struct/enum. Concrete engine classes (registered, unlike UStruct/UEnum which
            // are only registered under their reflection names "ScriptStruct"/"Enum").
            Register<Obj::Engine::UUserDefinedStruct>("UserDefinedStruct");
            Register<Obj::Engine::UUserDefinedEnum>("UserDefinedEnum");
            // Curve-asset exports (UCurveBase is abstract -> not registered). Each wraps the S25 FRichCurve/
            // FSimpleCurve eval subsystem; their curves come from the object's own tagged StructProperties.
            Register<Obj::Engine::Curves::UCurveFloat>("CurveFloat");
            Register<Obj::Engine::Curves::UCurveVector>("CurveVector");
            Register<Obj::Engine::Curves::UCurveLinearColor>("CurveLinearColor");

            // ---- Sound ------------------------------------------------------------------------------------
            // USoundBase is abstract in C# and so is deliberately absent; USoundCue/USoundWave derive from it.
            Register<Exports::Sound::UDialogueWave>("DialogueWave");
            Register<Exports::Sound::USoundClass>("SoundClass");
            Register<Exports::Sound::USoundCue>("SoundCue");
            Register<Exports::Sound::USoundWave>("SoundWave");
            Register<Exports::Sound::USoundWaveProcedural>("SoundWaveProcedural");
            Register<Exports::Sound::USoundSourceBus>("SoundSourceBus");
            // Exports/Sound/UMetaSoundSource.cs declares its class as `UMetaUSoundSource` — a typo in the C#
            // that reflection turns into the registered name "MetaUSoundSource", which no cooked asset uses.
            // The real MetaSound source registers below under "MetaSoundSource". Both are kept as they are:
            // "fixing" the name here would silently change which of the two types a package constructs.
            Register<Exports::Sound::UMetaUSoundSource>("MetaUSoundSource");

            // SoundCue's node graph. Every node type is a distinct serialized class in a cooked package.
            Register<Exports::Sound::Node::USoundNode>("SoundNode");
            Register<Exports::Sound::Node::USoundNodeAssetReferencer>("SoundNodeAssetReferencer");
            Register<Exports::Sound::Node::USoundNodeDialoguePlayer>("SoundNodeDialoguePlayer");
            Register<Exports::Sound::Node::USoundNodeWave>("SoundNodeWave");
            Register<Exports::Sound::Node::USoundNodeWavePlayer>("SoundNodeWavePlayer");
            Register<Exports::Sound::Node::USoundNodeDoppler>("SoundNodeDoppler");
            Register<Exports::Sound::Node::USoundNodeAttenuation>("SoundNodeAttenuation");
            Register<Exports::Sound::Node::USoundNodeQualityLevel>("SoundNodeQualityLevel");
            Register<Exports::Sound::Node::USoundNodeEnveloper>("SoundNodeEnveloper");
            Register<Exports::Sound::Node::USoundNodeDelay>("SoundNodeDelay");
            Register<Exports::Sound::Node::USoundNodeMixer>("SoundNodeMixer");
            Register<Exports::Sound::Node::USoundNodeModulator>("SoundNodeModulator");
            Register<Exports::Sound::Node::USoundNodeModulatorContinuous>("SoundNodeModulatorContinuous");
            Register<Exports::Sound::Node::USoundNodeRandom>("SoundNodeRandom");
            Register<Exports::Sound::Node::USoundNodeDistanceCrossFade>("SoundNodeDistanceCrossFade");
            Register<Exports::Sound::Node::USoundNodeParamCrossFade>("SoundNodeParamCrossFade");
            Register<Exports::Sound::Node::USoundNodeSwitch>("SoundNodeSwitch");
            Register<Exports::Sound::Node::USoundNodeSoundClass>("SoundNodeSoundClass");
            Register<Exports::Sound::Node::USoundNodeLooping>("SoundNodeLooping");
            Register<Exports::Sound::Node::USoundNodeBranch>("SoundNodeBranch");

            // ---- MetaSound --------------------------------------------------------------------------------
            Register<Exports::MetaSound::UMetaSoundPatch>("MetaSoundPatch");
            Register<Exports::MetaSound::UMetaSoundSource>("MetaSoundSource");

            // ---- Wwise (the Audiokinetic UE integration's asset types) -------------------------------------
            // UAkAudioType is a plain (non-abstract) base in C#, so it registers too.
            Register<Exports::Wwise::UAkAudioType>("AkAudioType");
            Register<Exports::Wwise::UAkAcousticTexture>("AkAcousticTexture");
            Register<Exports::Wwise::UAkAudioBank>("AkAudioBank");
            Register<Exports::Wwise::UAkAudioDeviceShareSet>("AkAudioDeviceShareSet");
            Register<Exports::Wwise::UAkAudioEvent>("AkAudioEvent");
            Register<Exports::Wwise::UAkAuxBus>("AkAuxBus");
            Register<Exports::Wwise::UAkEffectShareSet>("AkEffectShareSet");
            Register<Exports::Wwise::UAkGroupValue>("AkGroupValue");
            Register<Exports::Wwise::UAkStateValue>("AkStateValue");
            Register<Exports::Wwise::UAkSwitchValue>("AkSwitchValue");
            Register<Exports::Wwise::UAkInitBank>("AkInitBank");
            Register<Exports::Wwise::UAkRtpc>("AkRtpc");
            Register<Exports::Wwise::UAkTrigger>("AkTrigger");
            Register<Exports::Wwise::UAkAssetData>("AkAssetData");
            Register<Exports::Wwise::UAkAssetDataWithMedia>("AkAssetDataWithMedia");
            Register<Exports::Wwise::UAkAssetDataSwitchContainer>("AkAssetDataSwitchContainer");
            Register<Exports::Wwise::UAkAudioEventData>("AkAudioEventData");
            Register<Exports::Wwise::UAkInitBankAssetData>("AkInitBankAssetData");
            Register<Exports::Wwise::UAkAssetPlatformData>("AkAssetPlatformData");
            Register<Exports::Wwise::UAkMediaAsset>("AkMediaAsset");
            Register<Exports::Wwise::UAkMediaAssetData>("AkMediaAssetData");
            Register<Exports::Wwise::UAkExternalMediaAsset>("AkExternalMediaAsset");
            Register<Exports::Wwise::UAkLocalizedMediaAsset>("AkLocalizedMediaAsset");
            Register<Exports::Wwise::UWwiseAssetLibrary>("WwiseAssetLibrary");
            // Per-game renames of the two bank/event types, kept because their serialized class names differ:
            // UWui* is The Awesome Adventures of Captain Spirit, UWwise* is Borderlands 3.
            Register<Exports::Wwise::UWuiBank>("WuiBank");
            Register<Exports::Wwise::UWwiseBank>("WwiseBank");
            Register<Exports::Wwise::UWuiEvent>("WuiEvent");
            Register<Exports::Wwise::UWwiseEvent>("WwiseEvent");

            // ---- FMOD -------------------------------------------------------------------------------------
            // Marker types: FMOD's UE plugin keeps everything in tagged properties, so registering them only
            // changes the constructed C++ type, not what is read.
            Register<Exports::FMod::UFMODBank>("FMODBank");
            Register<Exports::FMod::UFMODBankLookup>("FMODBankLookup");
            Register<Exports::FMod::UFMODBus>("FMODBus");
            Register<Exports::FMod::UFMODEvent>("FMODEvent");
            Register<Exports::FMod::UFMODSnapshot>("FMODSnapshot");
            Register<Exports::FMod::UFMODSnapshotReverb>("FMODSnapshotReverb");
            Register<Exports::FMod::UFMODVCA>("FMODVCA");

            // ---- Texture ----------------------------------------------------------------------------------
            // UTexture is abstract in C# and so is deliberately absent; every concrete leaf below registers.
            // Texture2D alone is the single largest class name in a real cooked game.
            Register<Exports::Texture::UTexture2D>("Texture2D");
            Register<Exports::Texture::UTextureCube>("TextureCube");
            Register<Exports::Texture::UTextureCubeArray>("TextureCubeArray");
            Register<Exports::Texture::UTexture2DArray>("Texture2DArray");
            Register<Exports::Texture::UVolumeTexture>("VolumeTexture");
            Register<Exports::Texture::UVirtualTexture2D>("VirtualTexture2D");
            Register<Exports::Texture::ULightMapTexture2D>("LightMapTexture2D");
            Register<Exports::Texture::ULightMapVirtualTexture2D>("LightMapVirtualTexture2D");
            Register<Exports::Texture::UShadowMapTexture2D>("ShadowMapTexture2D");
            Register<Exports::Texture::URuntimeVirtualTextureStreamingProxy>("RuntimeVirtualTextureStreamingProxy");
            Register<Exports::Texture::UCurveLinearColorAtlas>("CurveLinearColorAtlas");
            Register<Exports::Texture::UTerrainWeightMapTexture>("TerrainWeightMapTexture");
            Register<Exports::Texture::USubstanceAirTexture2D>("SubstanceAirTexture2D");
            Register<Exports::Texture::UTextureFlipBook>("TextureFlipBook");
            Register<Exports::Texture::UTextureLightProfile>("TextureLightProfile");
            Register<Exports::Texture::UTextureRenderTarget>("TextureRenderTarget");
            Register<Exports::Texture::UTextureRenderTarget2D>("TextureRenderTarget2D");
            Register<Exports::Texture::UTextureRenderTargetCube>("TextureRenderTargetCube");
            Register<Exports::Texture::UMediaTexture>("MediaTexture");
            Register<Exports::Texture::UBinkMediaTexture>("BinkMediaTexture");
            Register<Exports::Texture::UTextureMovie>("TextureMovie");
            // Per-game types whose payload is a ".map" blob instead of a mip chain.
            Register<Exports::Texture::UTextureProFXParent>("TextureProFXParent");
            Register<Exports::Texture::UTextureProFXChild>("TextureProFXChild");
            // Not a texture -- a Paper2D sprite that points at one -- but it lives in the same C# folder.
            Register<Exports::Texture::UPaperSprite>("PaperSprite");
            // The mip-data provider factories a texture finds through its AssetUserData, plus the plain
            // UAssetUserData base they derive from (concrete in C#, so it registers too).
            Register<Obj::Engine::UAssetUserData>("AssetUserData");
            Register<Exports::Texture::UTextureMipDataProviderFactory>("TextureMipDataProviderFactory");
            Register<Exports::Texture::UTextureAllMipDataProviderFactory>("TextureAllMipDataProviderFactory");
        }

        void EnsureRegistered()
        {
            static std::once_flag flag;
            std::call_once(flag, RegisterEngineTypes);
        }
    }

    void ObjectTypeRegistry::RegisterClass(const std::string& serializedName, Factory factory)
    {
        Registry()[serializedName] = std::move(factory);
    }

    ObjectTypeRegistry::Factory ObjectTypeRegistry::Get(const std::string& serializedName)
    {
        EnsureRegistered();
        auto& reg = Registry();
        auto it = reg.find(serializedName);
        if (it == reg.end() && serializedName.size() > 2 &&
            serializedName.compare(serializedName.size() - 2, 2, "_C") == 0)
        {
            it = reg.find(serializedName.substr(0, serializedName.size() - 2));
        }
        return it != reg.end() ? it->second : Factory{};
    }
}
