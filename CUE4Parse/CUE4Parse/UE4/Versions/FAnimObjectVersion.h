// Ported from CUE4Parse/UE4/Versions/FAnimObjectVersion.cs
// Custom serialization version for changes made in Dev-Anim stream
// C#'s `static class` becomes a namespace: `Type` and `GUID` keep their qualified spelling
// (FXxx::Type::Member also resolves, the enum being unscoped) and Get() stays a free function.
#pragma once

#include "EGame.h"
#include "VersionUtils.h"
#include "../Objects/Core/Misc/FGuid.h"
#include "../Readers/FArchive.h"

namespace CUE4Parse::UE4::Versions
{
    namespace FAnimObjectVersion
    {
        using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

        enum Type
        {
            // Before any version changes were made
            BeforeCustomVersionWasAdded,

            // Reworked how anim blueprint root nodes are recovered
            LinkTimeAnimBlueprintRootDiscovery,

            // Cached marker sync names on skeleton for editor
            StoreMarkerNamesOnSkeleton,

            // Serialized register array state for RigVM
            SerializeRigVMRegisterArrayState,

            // Increase number of bones per chunk from uint8 to uint16
            IncreaseBoneIndexLimitPerChunk,

            UnlimitedBoneInfluences,

            // Anim sequences have colors for their curves
            AnimSequenceCurveColors,

            // Notifies and sync markers now have Guids
            NotifyAndSyncMarkerGuids,

            // Serialized register dynamic state for RigVM
            SerializeRigVMRegisterDynamicState,

            // Groom cards serialization
            SerializeGroomCards,

            // Serialized rigvm entry names
            SerializeRigVMEntries,

            // Serialized rigvm entry names
            SerializeHairBindingAsset,

            // Serialized rigvm entry names
            SerializeHairClusterCullingData,

            // Groom cards and meshes serialization
            SerializeGroomCardsAndMeshes,

            // Stripping LOD data from groom
            GroomLODStripping,

            // Stripping LOD data from groom
            GroomBindingSerialization,

            // -----<new versions can be added above this line>-------------------------------------------------
            VersionPlusOne,
            LatestVersion = VersionPlusOne - 1,
        };

        inline const FGuid GUID(0xAF43A65D, 0x7FD34947, 0x98733E8E, 0xD9C1BB05);

        inline Type Get(Readers::FArchive& Ar)
        {
            const auto ver = CustomVer(Ar, GUID);
            if (ver >= 0) return static_cast<Type>(ver);

            const EGame game = Ar.Game();
            if (game == GAME_DeltaForce) return StoreMarkerNamesOnSkeleton;
            if (game < GAME_UE4_21) return BeforeCustomVersionWasAdded;
            if (game < GAME_UE4_25) return StoreMarkerNamesOnSkeleton;
            if (game < GAME_UE4_26) return NotifyAndSyncMarkerGuids;
            return LatestVersion;
        }
    }
}
