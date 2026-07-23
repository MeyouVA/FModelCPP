// Ported from CUE4Parse/UE4/Versions/FFoliageCustomVersion.cs
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
    namespace FFoliageCustomVersion
    {
        using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

        enum Type
        {
            // Before any version changes were made in the plugin
            BeforeCustomVersionWasAdded = 0,
            // Converted to use HierarchicalInstancedStaticMeshComponent
            FoliageUsingHierarchicalISMC = 1,
            // Changed Component to not RF_Transactional
            HierarchicalISMCNonTransactional = 2,
            // Added FoliageTypeUpdateGuid
            AddedFoliageTypeUpdateGuid = 3,
            // Use a GUID to determine whic procedural actor spawned us
            ProceduralGuid = 4,
            // Support for cross-level bases
            CrossLevelBase = 5,
            // FoliageType for details customization
            FoliageTypeCustomization = 6,
            // FoliageType for details customization continued
            FoliageTypeCustomizationScaling = 7,
            // FoliageType procedural scale and shade settings updated
            FoliageTypeProceduralScaleAndShade = 8,
            // Added FoliageHISMC and blueprint support
            FoliageHISMCBlueprints = 9,
            // Added Mobility setting to UFoliageType
            AddedMobility = 10,
            // Make sure that foliage has FoliageHISMC class
            FoliageUsingFoliageISMC = 11,
            // Foliage Actor Support
            FoliageActorSupport = 12,
            // Foliage Actor (No weak ptr)
            FoliageActorSupportNoWeakPtr = 13,
            // Foliage Instances are now always saved local to Level
            FoliageRepairInstancesWithLevelTransform = 14,
            // Supports discarding foliage types on load independently from density scaling
            FoliageDiscardOnLoad = 15,

            // -----<new versions can be added above this line>-------------------------------------------------
            VersionPlusOne,
            LatestVersion = VersionPlusOne - 1,
        };

        inline const FGuid GUID(0x430C4D19, 0x71544970, 0x87699B69, 0xDF90B0E5);

        inline Type Get(Readers::FArchive& Ar)
        {
            const auto ver = CustomVer(Ar, GUID);
            if (ver >= 0) return static_cast<Type>(ver);

            const EGame game = Ar.Game();
            if (game < GAME_UE4_7) return BeforeCustomVersionWasAdded;
            if (game < GAME_UE4_8) return AddedFoliageTypeUpdateGuid;
            if (game < GAME_UE4_9) return FoliageTypeProceduralScaleAndShade;
            if (game < GAME_UE4_10) return AddedMobility;
            if (game < GAME_UE4_23) return FoliageUsingFoliageISMC;
            if (game < GAME_UE4_24) return FoliageActorSupportNoWeakPtr;
            if (game < GAME_UE4_26) return FoliageRepairInstancesWithLevelTransform;
            return LatestVersion;
        }
    }
}
