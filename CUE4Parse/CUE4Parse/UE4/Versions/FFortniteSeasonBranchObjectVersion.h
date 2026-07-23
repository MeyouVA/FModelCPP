// Ported from CUE4Parse/UE4/Versions/FFortniteSeasonBranchObjectVersion.cs
// Custom serialization version for changes made in the //Fortnite/Dev-FN-Sxx stream
// C#'s `static class` becomes a namespace: `Type` and `GUID` keep their qualified spelling
// (FXxx::Type::Member also resolves, the enum being unscoped) and Get() stays a free function.
#pragma once

#include "EGame.h"
#include "VersionUtils.h"
#include "../Objects/Core/Misc/FGuid.h"
#include "../Readers/FArchive.h"

namespace CUE4Parse::UE4::Versions
{
    namespace FFortniteSeasonBranchObjectVersion
    {
        using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

        enum Type
        {
            // Before any version changes were made
            BeforeCustomVersionWasAdded = 0,

            // Added FWorldDataLayersActorDesc
            AddedWorldDataLayersActorDesc,

            // Fixed FDataLayerInstanceDesc
            FixedDataLayerInstanceDesc,

            // Serialize DataLayerAssets in WorldPartitionActorDesc
            WorldPartitionActorDescSerializeDataLayerAssets,

            // Remapped bEvaluateWorldPositionOffset to bEvaluateWorldPositionOffsetInRayTracing
            RemappedEvaluateWorldPositionOffsetInRayTracing,

            // Serialize native and base class for actor descriptors
            WorldPartitionActorDescNativeBaseClassSerialization,

            // Serialize tags for actor descriptors
            WorldPartitionActorDescTagsSerialization,

            // Serialize property map for actor descriptors
            WorldPartitionActorDescPropertyMapSerialization,

            // Added ability to mark shapes as probes
            AddShapeIsProbe,

            // Transfer PhysicsAsset SolverSettings (iteration counts etc) to new structure
            PhysicsAssetNewSolverSettings,

            // Chaos GeometryCollection now saves levels attribute values
            ChaosGeometryCollectionSaveLevelsAttribute,

            // Serialize actor transform for actor descriptors
            WorldPartitionActorDescActorTransformSerialization,

            // Changing Chaos::FImplicitObjectUnion to store an int32 vs a uint16 for NumLeafObjects.
            ChaosImplicitObjectUnionLeafObjectsToInt32,

            // Chaos Visual Debugger : Adding serialization for properties that were being recorded, but not serialized
            CVDSerializationFixMissingSerializationProperties,

            // Updated Enhanceed Input Mapping Contexts to support adding "Profile override" mappings.
            EnhancedInputMappingContextProfileMappingsUpdate,

            // Introduce per entity support for external owned entities
            SceneGraphExternalOwnedEntities,

            // -----<new versions can be added above this line>-------------------------------------------------
            VersionPlusOne,
            LatestVersion = VersionPlusOne - 1
        };

        inline const FGuid GUID(0x5B4C06B7, 0x24634AF8, 0x805BBF70, 0xCDF5D0DD);

        inline Type Get(Readers::FArchive& Ar)
        {
            const auto ver = CustomVer(Ar, GUID);
            if (ver >= 0) return static_cast<Type>(ver);

            const EGame game = Ar.Game();
            if (game < GAME_UE5_1) return BeforeCustomVersionWasAdded;
            if (game < GAME_UE5_4) return ChaosGeometryCollectionSaveLevelsAttribute;
            if (game < GAME_UE5_5) return ChaosImplicitObjectUnionLeafObjectsToInt32;
            if (game < GAME_UE5_7) return CVDSerializationFixMissingSerializationProperties;
            return LatestVersion;
        }
    }
}
