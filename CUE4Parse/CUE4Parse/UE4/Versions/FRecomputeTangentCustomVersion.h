// Ported from CUE4Parse/UE4/Versions/FRecomputeTangentCustomVersion.cs
// Custom serialization version for RecomputeTangent
// C#'s `static class` becomes a namespace: `Type` and `GUID` keep their qualified spelling
// (FXxx::Type::Member also resolves, the enum being unscoped) and Get() stays a free function.
#pragma once

#include "EGame.h"
#include "VersionUtils.h"
#include "../Objects/Core/Misc/FGuid.h"
#include "../Readers/FArchive.h"

namespace CUE4Parse::UE4::Versions
{
    namespace FRecomputeTangentCustomVersion
    {
        using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

        enum Type
        {
            // Before any version changes were made in the plugin
            BeforeCustomVersionWasAdded = 0,
            // UE4.12
            // We serialize the RecomputeTangent Option
            RuntimeRecomputeTangent = 1,
            // UE4.26
            // Choose which Vertex Color channel to use as mask to blend tangents
            RecomputeTangentVertexColorMask = 2,
            // -----<new versions can be added above this line>-------------------------------------------------
            VersionPlusOne,
            LatestVersion = VersionPlusOne - 1
        };

        inline const FGuid GUID(0x5579F886, 0x933A4C1F, 0x83BA087B, 0x6361B92F);

        inline Type Get(Readers::FArchive& Ar)
        {
            const auto ver = CustomVer(Ar, GUID);
            if (ver >= 0) return static_cast<Type>(ver);

            const EGame game = Ar.Game();
            if (game < GAME_UE4_12) return BeforeCustomVersionWasAdded;
            if (game < GAME_UE4_26) return RuntimeRecomputeTangent;
            return RecomputeTangentVertexColorMask;
        }
    }
}
