// Ported from CUE4Parse/UE4/Versions/FDestructionObjectVersion.cs
// Custom serialization version for changes made in Dev-Destruction stream
// C#'s `static class` becomes a namespace: `Type` and `GUID` keep their qualified spelling
// (FXxx::Type::Member also resolves, the enum being unscoped) and Get() stays a free function.
#pragma once

#include "EGame.h"
#include "VersionUtils.h"
#include "../Objects/Core/Misc/FGuid.h"
#include "../Readers/FArchive.h"

namespace CUE4Parse::UE4::Versions
{
    namespace FDestructionObjectVersion
    {
        using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

        enum Type
        {
            // Before any version changes were made
            BeforeCustomVersionWasAdded = 0,

            // Added timestamped caches for geometry component to handle transform sampling instead of per-frame
            AddedTimestampedGeometryComponentCache,

            // Added functionality to strip unnecessary data from geometry collection caches
            AddedCacheDataReduction,

            // Geometry collection data is now in the DDC
            GeometryCollectionInDDC,

            // Geometry collection data is now in both the DDC and the asset
            GeometryCollectionInDDCAndAsset,

            // New way to serialize unique ptr and serializable ptr
            ChaosArchiveAdded,

            // Serialization support for UFieldSystems
            FieldsAdded,

            // density default units changed from kg/cm3 to kg/m3
            DensityUnitsChanged,

            // bulk serialize arrays
            BulkSerializeArrays,

            // bulk serialize arrays
            GroupAndAttributeNameRemapping,

            // bulk serialize arrays
            ImplicitObjectDoCollideAttribute,


            // -----<new versions can be added above this line>-------------------------------------------------
            VersionPlusOne,
            LatestVersion = VersionPlusOne - 1
        };

        inline const FGuid GUID(0x174F1F0B, 0xB4C645A5, 0xB13F2EE8, 0xD0FB917D);

        inline Type Get(Readers::FArchive& Ar)
        {
            const auto ver = CustomVer(Ar, GUID);
            if (ver >= 0) return static_cast<Type>(ver);

            const EGame game = Ar.Game();
            if (game < GAME_UE4_22) return BeforeCustomVersionWasAdded;
            if (game < GAME_UE4_23) return AddedCacheDataReduction;
            if (game < GAME_UE4_25) return GroupAndAttributeNameRemapping;
            return LatestVersion;
        }
    }
}
