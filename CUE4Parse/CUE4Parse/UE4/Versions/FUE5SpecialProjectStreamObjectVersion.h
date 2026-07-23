// Ported from CUE4Parse/UE4/Versions/FUE5SpecialProjectStreamObjectVersion.cs
// Custom serialization version for changes made in //UE5/Private-Frosty stream
// C#'s `static class` becomes a namespace: `Type` and `GUID` keep their qualified spelling
// (FXxx::Type::Member also resolves, the enum being unscoped) and Get() stays a free function.
#pragma once

#include "EGame.h"
#include "VersionUtils.h"
#include "../Objects/Core/Misc/FGuid.h"
#include "../Readers/FArchive.h"

namespace CUE4Parse::UE4::Versions
{
    namespace FUE5SpecialProjectStreamObjectVersion
    {
        using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

        enum Type
        {
            // Before any version changes were made
            BeforeCustomVersionWasAdded = 0,

            // Added HLODBatchingPolicy member to UPrimitiveComponent, which replaces the confusing bUseMaxLODAsImposter & bBatchImpostersAsInstances.
            HLODBatchingPolicy,

            // Serialize scene components static bounds
            SerializeSceneComponentStaticBounds,

            // Add the long range attachment tethers to the cloth asset to avoid a large hitch during the cloth's initialization.
            ChaosClothAddTethersToCachedData,

            // Always serialize the actor label in cooked builds
            SerializeActorLabelInCookedBuilds,

            // Changed world partition HLODs cells from FSotObjectPath to FName
            ConvertWorldPartitionHLODsCellsToName,

            // Re-calculate the long range attachment to prevent kinematic tethers.
            ChaosClothRemoveKinematicTethers,

            // Serializes the Morph Target render data for cooked platforms and the DDC
            SerializeSkeletalMeshMorphTargetRenderData,

            // Strip the Morph Target source data for cooked builds
            StripMorphTargetSourceDataForCookedBuilds,

            // StateTree now holds PropertyBag + GUID for root-level parameters rather than FStateTreeStateParameters. Access is protected by default and can be overriden through virtuals on UStateTreeEditorData derived classes.
            StateTreeGlobalParameterChanges,

            // -----<new versions can be added above this line>-------------------------------------------------
            VersionPlusOne,
            LatestVersion = VersionPlusOne - 1
        };

        inline const FGuid GUID(0x59DA5D52, 0x12324948, 0xB8785978, 0x70B8E98B);

        inline Type Get(Readers::FArchive& Ar)
        {
            const auto ver = CustomVer(Ar, GUID);
            if (ver >= 0) return static_cast<Type>(ver);

            const EGame game = Ar.Game();
            if (game < GAME_UE5_0) return BeforeCustomVersionWasAdded;
            if (game < GAME_UE5_6) return StripMorphTargetSourceDataForCookedBuilds;
            return LatestVersion;
        }
    }
}
