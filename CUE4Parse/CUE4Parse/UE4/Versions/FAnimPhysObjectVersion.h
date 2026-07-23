// Ported from CUE4Parse/UE4/Versions/FAnimPhysObjectVersion.cs
// Custom serialization version for changes made in Dev-AnimPhys stream
// C#'s `static class` becomes a namespace: `Type` and `GUID` keep their qualified spelling
// (FXxx::Type::Member also resolves, the enum being unscoped) and Get() stays a free function.
#pragma once

#include "EGame.h"
#include "VersionUtils.h"
#include "../Objects/Core/Misc/FGuid.h"
#include "../Readers/FArchive.h"

namespace CUE4Parse::UE4::Versions
{
    namespace FAnimPhysObjectVersion
    {
        using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

        enum Type
        {
            // Before any version changes were made
            BeforeCustomVersionWasAdded,
            // convert animnode look at to use just default axis instead of enum, which doesn't do much
            ConvertAnimNodeLookAtAxis,
            // Change FKSphylElem and FKBoxElem to use Rotators not Quats for easier editing
            BoxSphylElemsUseRotators,
            // Change thumbnail scene info and asset import data to be transactional
            ThumbnailSceneInfoAndAssetImportDataAreTransactional,
            // Enabled clothing masks rather than painting parameters directly
            AddedClothingMaskWorkflow,
            // Remove UID from smart name serialize, it just breaks determinism
            RemoveUIDFromSmartNameSerialize,
            // Convert FName Socket to FSocketReference and added TargetReference that support bone and socket
            CreateTargetReference,
            // Tune soft limit stiffness and damping coefficients
            TuneSoftLimitStiffnessAndDamping,
            // Fix possible inf/nans in clothing particle masses
            FixInvalidClothParticleMasses,
            // Moved influence count to cached data
            CacheClothMeshInfluences,
            // Remove GUID from Smart Names entirely + remove automatic name fixup
            SmartNameRefactorForDeterministicCooking,
            // rename the variable and allow individual curves to be set
            RenameDisableAnimCurvesToAllowAnimCurveEvaluation,
            // link curve to LOD, so curve metadata has to include LODIndex
            AddLODToCurveMetaData,
            // Fixed blend profile references persisting after paste when they aren't compatible
            FixupBadBlendProfileReferences,
            // Allowing multiple audio plugin settings
            AllowMultipleAudioPluginSettings,
            // Change RetargetSource reference to SoftObjectPtr
            ChangeRetargetSourceReferenceToSoftObjectPtr,
            // Save editor only full pose for pose asset
            SaveEditorOnlyFullPoseForPoseAsset,
            // Asset change and cleanup to facilitate new streaming system
            GeometryCacheAssetDeprecation,

            // -----<new versions can be added above this line>-------------------------------------------------
            VersionPlusOne,
            LatestVersion = VersionPlusOne - 1,
        };

        inline const FGuid GUID(0x29E575DD, 0xE0A34627, 0x9D10D276, 0x232CDCEA);

        inline Type Get(Readers::FArchive& Ar)
        {
            const auto ver = CustomVer(Ar, GUID);
            if (ver >= 0) return static_cast<Type>(ver);

            const EGame game = Ar.Game();
            if (game < GAME_UE4_16) return BeforeCustomVersionWasAdded;
            if (game < GAME_UE4_17) return ThumbnailSceneInfoAndAssetImportDataAreTransactional;
            if (game < GAME_UE4_18) return TuneSoftLimitStiffnessAndDamping;
            if (game < GAME_UE4_19) return AddLODToCurveMetaData;
            if (game < GAME_UE4_20) return SaveEditorOnlyFullPoseForPoseAsset;
            if (game < GAME_UE4_26) return GeometryCacheAssetDeprecation;
            return LatestVersion;
        }
    }
}
