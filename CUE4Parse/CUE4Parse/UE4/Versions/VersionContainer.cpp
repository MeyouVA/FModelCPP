// Ported from CUE4Parse/UE4/Versions/VersionContainer.cs (InitOptions / InitMapStructTypes / the indexer).
#include "VersionContainer.h"

#include <stdexcept>

namespace CUE4Parse::UE4::Versions
{
    bool VersionContainer::operator[](const std::string& optionKey) const
    {
        const auto it = Options.find(optionKey);
        if (it == Options.end())
            throw std::out_of_range("Unknown version option '" + optionKey + "'");
        return it->second;
    }

    bool VersionContainer::OverrideUseAudioStreaming() const
    {
        return !(_game == GAME_UE4_28 || _game == GAME_GTATheTrilogyDefinitiveEdition ||
                 _game == GAME_ReadyOrNot || _game == GAME_BladeAndSoul || _game == GAME_Stray);
    }

    void VersionContainer::InitOptions()
    {
        Options.clear();

        // objects
        Options["MorphTarget"] = true;

        // structs
        Options["Vector_NetQuantize_AsStruct"] = _game >= GAME_UE5_0;

        // fields
        Options["RawIndexBuffer.HasShouldExpandTo32Bit"] =
            _game >= GAME_UE4_25 && _game != GAME_DeltaForce && _game != GAME_ArenaBreakoutMobile;
        Options["ShaderMap.UseNewCookedFormat"] = _game >= GAME_UE5_0;
        Options["SkeletalMesh.UseNewCookedFormat"] = _game >= GAME_UE4_24;
        Options["SkeletalMesh.HasRayTracingData"] = _game >= GAME_UE4_27 || _game == GAME_UE4_25_Plus;
        // Exists in all engine versions except UE4.15
        Options["StaticMesh.HasLODsShareStaticLighting"] = _game < GAME_UE4_15 || _game >= GAME_UE4_16;
        Options["StaticMesh.HasRayTracingGeometry"] = _game >= GAME_UE4_25;
        Options["StaticMesh.HasVisibleInRayTracing"] = _game >= GAME_UE4_26 || _game == GAME_Back4Blood;
        Options["StaticMesh.UseNewCookedFormat"] = _game >= GAME_UE4_23;
        Options["VirtualTextures"] = _game >= GAME_UE4_23;
        // A lot of games use this, but some don't, which causes issues.
        Options["SoundWave.UseAudioStreaming"] = _game >= GAME_UE4_25 && OverrideUseAudioStreaming();
        // Early 4.17 builds don't have this, and some custom engine builds don't either.
        Options["AnimSequence.HasCompressedRawSize"] = _game >= GAME_UE4_17;
        Options["StaticMesh.HasNavCollision"] =
            _ver >= EUnrealEngineObjectUE4Version::STATIC_MESH_STORE_NAV_COLLISION &&
            _game != GAME_GearsOfWar4 && _game != GAME_TEKKEN7;

        // special general property workarounds
        Options["ByteProperty.TMap64Bit"] = false;
        Options["ByteProperty.TMap16Bit"] = false;
        Options["ByteProperty.TMap8Bit"] = false;

        // defaults
        Options["StripAdditiveRefPose"] = false;
        Options["SkeletalMesh.KeepMobileMinLODSettingOnDesktop"] = false;
        Options["StaticMesh.KeepMobileMinLODSettingOnDesktop"] = false;

        for (const auto& [key, value] : _optionOverrides)
            Options[key] = value;
    }

    void VersionContainer::InitMapStructTypes()
    {
        MapStructTypes.clear();
        MapStructTypes["BindingIdToReferences"] = {"Guid", ""};
        MapStructTypes["UserParameterRedirects"] = {"NiagaraVariable", "NiagaraVariable"};
        MapStructTypes["Tracks"] = {"MovieSceneTrackIdentifier", ""};
        MapStructTypes["SubSequences"] = {"MovieSceneSequenceID", ""};
        MapStructTypes["Hierarchy"] = {"MovieSceneSequenceID", ""};
        MapStructTypes["TrackSignatureToTrackIdentifier"] = _game < GAME_UE4_19
            ? FMapStructTypes{"Guid", "MovieSceneTrackIdentifiers"}
            : FMapStructTypes{"Guid", "MovieSceneTrackIdentifier"};

        for (const auto& [key, value] : _mapStructTypesOverrides)
            MapStructTypes[key] = value;
    }
}
