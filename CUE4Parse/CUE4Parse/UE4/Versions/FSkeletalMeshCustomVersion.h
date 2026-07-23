// Ported from CUE4Parse/UE4/Versions/FSkeletalMeshCustomVersion.cs
// Custom serialization version for SkeletalMesh types
// C#'s `static class` becomes a namespace: `Type` and `GUID` keep their qualified spelling
// (FXxx::Type::Member also resolves, the enum being unscoped) and Get() stays a free function.
#pragma once

#include "EGame.h"
#include "VersionUtils.h"
#include "../Objects/Core/Misc/FGuid.h"
#include "../Readers/FArchive.h"

namespace CUE4Parse::UE4::Versions
{
    namespace FSkeletalMeshCustomVersion
    {
        using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

        enum Type
        {
            // Before any version changes were made
            BeforeCustomVersionWasAdded = 0,
            // UE4.13 = 4
            // Remove Chunks array in FStaticLODModel and combine with Sections array
            CombineSectionWithChunk = 1,
            // Remove FRigidSkinVertex and combine with FSoftSkinVertex array
            CombineSoftAndRigidVerts = 2,
            // Need to recalc max bone influences
            RecalcMaxBoneInfluences = 3,
            // Add NumVertices that can be accessed when stripping editor data
            SaveNumVertices = 4,
            // UE4.14 = 5
            // Regenerated clothing section shadow flags from source sections
            RegenerateClothingShadowFlags = 5,
            // UE4.15 = 7
            // Share color buffer structure with StaticMesh
            UseSharedColorBufferFormat = 6,
            // Use separate buffer for skin weights
            UseSeparateSkinWeightBuffer = 7,
            // UE4.16, UE4.17 = 9
            // Added new clothing systems
            NewClothingSystemAdded = 8,
            // Cached inv mass data for clothing assets
            CachedClothInverseMasses = 9,
            // UE4.18 = 10
            // Compact cloth vertex buffer, without dummy entries
            CompactClothVertexBuffer = 10,
            // UE4.19 = 15
            // Remove SourceData
            RemoveSourceData = 11,
            // Split data into Model and RenderData
            SplitModelAndRenderData = 12,
            // Remove triangle sorting support
            RemoveTriangleSorting = 13,
            // Remove the duplicated clothing sections that were a legacy holdover from when we didn't use our own render data
            RemoveDuplicatedClothingSections = 14,
            // Remove 'Disabled' flag from SkelMesh asset sections
            DeprecateSectionDisabledFlag = 15,
            // UE4.20-UE4.22 = 16
            // Add Section ignore by reduce
            SectionIgnoreByReduceAdded = 16,
            // UE4.23-UE4.25 = 17
            // Adding skin weight profile support
            SkinWeightProfiles = 17, // TODO: FSkeletalMeshLODModel::Serialize (editor mesh)
            // UE4.26 = 18
            // Remove uninitialized/deprecated enable cloth LOD flag
            RemoveEnableClothLOD = 18,

            // -----<new versions can be added above this line>-------------------------------------------------
            VersionPlusOne,
            LatestVersion = VersionPlusOne - 1
        };

        inline const FGuid GUID(0xD78A4A00, 0xE8584697, 0xBAA819B5, 0x487D46B4);

        inline Type Get(Readers::FArchive& Ar)
        {
            const auto ver = CustomVer(Ar, GUID);
            if (ver >= 0) return static_cast<Type>(ver);

            const EGame game = Ar.Game();
            if (game == GAME_Paragon) return SplitModelAndRenderData;
            if (game < GAME_UE4_13) return BeforeCustomVersionWasAdded;
            if (game < GAME_UE4_14) return SaveNumVertices;
            if (game < GAME_UE4_15) return RegenerateClothingShadowFlags;
            if (game < GAME_UE4_16) return UseSeparateSkinWeightBuffer;
            if (game < GAME_UE4_18) return CachedClothInverseMasses;
            if (game < GAME_UE4_19) return CompactClothVertexBuffer;
            if (game < GAME_UE4_20) return DeprecateSectionDisabledFlag;
            if (game < GAME_UE4_23) return SectionIgnoreByReduceAdded;
            if (game < GAME_UE4_26) return SkinWeightProfiles;
            return RemoveEnableClothLOD;
        }
    }
}
