// Ported from CUE4Parse/UE4/Versions/FMobileObjectVersion.cs
// Custom serialization version for changes made in Dev-Mobile stream
// C#'s `static class` becomes a namespace: `Type` and `GUID` keep their qualified spelling
// (FXxx::Type::Member also resolves, the enum being unscoped) and Get() stays a free function.
#pragma once

#include "EGame.h"
#include "VersionUtils.h"
#include "../Objects/Core/Misc/FGuid.h"
#include "../Readers/FArchive.h"

namespace CUE4Parse::UE4::Versions
{
    namespace FMobileObjectVersion
    {
        using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

        enum Type
        {
            // Before any version changes were made
            BeforeCustomVersionWasAdded = 0,

            // Removed LightmapUVBias, ShadowmapUVBias from per-instance data
            InstancedStaticMeshLightmapSerialization,

            // Added stationary point/spot light direct contribution to volumetric lightmaps.
            LQVolumetricLightmapLayers,

            // Store Reflection Capture in compressed format for mobile
            StoreReflectionCaptureCompressedMobile,

            // -----<new versions can be added above this line>-------------------------------------------------
            VersionPlusOne,
            LatestVersion = VersionPlusOne - 1
        };

        inline const FGuid GUID(0xB02B49B5, 0xBB2044E9, 0xA30432B7, 0x52E40360);

        inline Type Get(Readers::FArchive& Ar)
        {
            const auto ver = CustomVer(Ar, GUID);
            if (ver >= 0) return static_cast<Type>(ver);

            const EGame game = Ar.Game();
            if (game < GAME_UE4_19) return BeforeCustomVersionWasAdded;
            if (game < GAME_UE4_26) return LQVolumetricLightmapLayers;
            return LatestVersion;
        }
    }
}
