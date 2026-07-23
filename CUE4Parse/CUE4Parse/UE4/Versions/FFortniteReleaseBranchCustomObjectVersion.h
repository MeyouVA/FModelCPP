// Ported from CUE4Parse/UE4/Versions/FFortniteReleaseBranchCustomObjectVersion.cs
// Custom serialization version for changes made in the //Fortnite/Release-XX.XX stream
// C#'s `static class` becomes a namespace: `Type` and `GUID` keep their qualified spelling
// (FXxx::Type::Member also resolves, the enum being unscoped) and Get() stays a free function.
#pragma once

#include "EGame.h"
#include "VersionUtils.h"
#include "../Objects/Core/Misc/FGuid.h"
#include "../Readers/FArchive.h"

namespace CUE4Parse::UE4::Versions
{
    namespace FFortniteReleaseBranchCustomObjectVersion
    {
        using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

        enum Type
        {
            // Before any version changes were made
            BeforeCustomVersionWasAdded = 0,

            // Custom 14.10 File Object Version
            DisableLevelset_v14_10,

            // Add the long range attachment tethers to the cloth asset to avoid a large hitch during the cloth's initialization.
            ChaosClothAddTethersToCachedData,

            // Chaos::TKinematicTarget no longer stores a full transform, only position/rotation.
            ChaosKinematicTargetRemoveScale,

            // Move UCSModifiedProperties out of ActorComponent and in to sparse storage
            ActorComponentUCSModifiedPropertiesSparseStorage,

            // Fixup Nanite meshes which were using the wrong material and didn't have proper UVs :
            FixupNaniteLandscapeMeshes,

            // Remove any cooked collision data from nanite landscape / editor spline meshes since collisions are not needed there :
            RemoveUselessLandscapeMeshesCookedCollisionData,

            // Serialize out UAnimCurveCompressionCodec::InstanceGUID to maintain deterministic DDC key generation in cooked-editor
            SerializeAnimCurveCompressionCodecGuidOnCook,

            // Fix the Nanite landscape mesh being reused because of a bad name
            FixNaniteLandscapeMeshNames,

            // Fixup and synchronize shared properties modified before the synchronicity enforcement
            LandscapeSharedPropertiesEnforcement,

            // Include the cell size when computing the cell guid
            WorldPartitionRuntimeCellGuidWithCellSize,

            // Enable SkipOnlyEditorOnly style cooking of NaniteOverrideMaterial
            NaniteMaterialOverrideUsesEditorOnly,

            // Store game thread particles data in single precision
            SinglePrecisionParticleData,

            // UPCGPoint custom serialization
            PCGPointStructuredSerializer,

            // Deprecation of Nav Movement Properties and moving them to a new struct
            NavMovementComponentMovingPropertiesToStruct,

            // Add bone serialization for dynamic mesh attributes
            DynamicMeshAttributesSerializeBones,

            // Add option for sanitizing output attribute names for all PCG data getters
            OptionSanitizeOutputAttributeNamesPCG,

            // Add automatic platform naming fix up for CommonUI input action data tables
            CommonUIPlatformNamingUpgradeOption,

            // -----<new versions can be added above this line>-------------------------------------------------
            VersionPlusOne,
            LatestVersion = VersionPlusOne - 1
        };

        inline const FGuid GUID(0xE7086368, 0x6B234C58, 0x84391B70, 0x16265E91);

        inline Type Get(Readers::FArchive& Ar)
        {
            const auto ver = CustomVer(Ar, GUID);
            if (ver >= 0) return static_cast<Type>(ver);

            const EGame game = Ar.Game();
            if (game < GAME_UE4_25) return BeforeCustomVersionWasAdded;
            if (game < GAME_UE5_0) return DisableLevelset_v14_10;
            if (game < GAME_UE5_1) return ChaosKinematicTargetRemoveScale;
            if (game < GAME_UE5_2) return ActorComponentUCSModifiedPropertiesSparseStorage;
            if (game < GAME_UE5_3) return RemoveUselessLandscapeMeshesCookedCollisionData;
            if (game < GAME_UE5_4) return NaniteMaterialOverrideUsesEditorOnly;
            if (game < GAME_UE5_5) return PCGPointStructuredSerializer;
            if (game < GAME_UE5_6) return DynamicMeshAttributesSerializeBones;
            return LatestVersion;
        }
    }
}
