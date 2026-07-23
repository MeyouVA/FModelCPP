// Ported from CUE4Parse/UE4/Versions/FSequencerObjectVersion.cs
// Custom serialization version for changes made in Dev-Sequencer stream
// C#'s `static class` becomes a namespace: `Type` and `GUID` keep their qualified spelling
// (FXxx::Type::Member also resolves, the enum being unscoped) and Get() stays a free function.
#pragma once

#include "EGame.h"
#include "VersionUtils.h"
#include "../Objects/Core/Misc/FGuid.h"
#include "../Readers/FArchive.h"

namespace CUE4Parse::UE4::Versions
{
    namespace FSequencerObjectVersion
    {
        using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

        enum Type
        {
            // Before any version changes were made
            BeforeCustomVersionWasAdded = 0,

            // Per-platform overrides player overrides for media sources changed name and type.
            RenameMediaSourcePlatformPlayers,

            // Enable root motion isn't the right flag to use, but force root lock
            ConvertEnableRootMotionToForceRootLock,

            // Convert multiple rows to tracks
            ConvertMultipleRowsToTracks,

            // When finished now defaults to restore state
            WhenFinishedDefaultsToRestoreState,

            // EvaluationTree added
            EvaluationTree,

            // When finished now defaults to project default
            WhenFinishedDefaultsToProjectDefault,

            // When finished now defaults to project default
            FloatToIntConversion,

            // Purged old spawnable blueprint classes from level sequence assets
            PurgeSpawnableBlueprints,

            // Finish UMG evaluation on end
            FinishUMGEvaluation,

            // Manual serialization of float channel
            SerializeFloatChannel,

            // Change the linear keys so they act the old way and interpolate always.
            ModifyLinearKeysForOldInterp,

            // Full Manual serialization of float channel
            SerializeFloatChannelCompletely,

            // Set ContinuouslyRespawn to false by default, added FMovieSceneSpawnable::bNetAddressableName
            SpawnableImprovements,

            // -----<new versions can be added above this line>-------------------------------------------------
            VersionPlusOne,
            LatestVersion = VersionPlusOne - 1
        };

        inline const FGuid GUID(0x7B5AE74C, 0xD2704C10, 0xA9585798, 0x0B212A5A);

        inline Type Get(Readers::FArchive& Ar)
        {
            const auto ver = CustomVer(Ar, GUID);
            if (ver >= 0) return static_cast<Type>(ver);

            const EGame game = Ar.Game();
            if (game == GAME_DeltaForce) return ModifyLinearKeysForOldInterp;
            if (game < GAME_UE4_14) return BeforeCustomVersionWasAdded;
            if (game < GAME_UE4_15) return RenameMediaSourcePlatformPlayers;
            if (game < GAME_UE4_16) return ConvertMultipleRowsToTracks;
            if (game < GAME_UE4_19) return WhenFinishedDefaultsToRestoreState;
            if (game < GAME_UE4_20) return WhenFinishedDefaultsToProjectDefault;
            if (game < GAME_UE4_22) return FinishUMGEvaluation;
            if (game < GAME_UE4_25) return ModifyLinearKeysForOldInterp;
            if (game < GAME_UE4_27) return SerializeFloatChannelCompletely;
            return LatestVersion;
        }
    }
}
