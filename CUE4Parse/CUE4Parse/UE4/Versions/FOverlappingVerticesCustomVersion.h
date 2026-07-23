// Ported from CUE4Parse/UE4/Versions/FOverlappingVerticesCustomVersion.cs
// custom version for overlapping vertcies code
// C#'s `static class` becomes a namespace: `Type` and `GUID` keep their qualified spelling
// (FXxx::Type::Member also resolves, the enum being unscoped) and Get() stays a free function.
#pragma once

#include "EGame.h"
#include "VersionUtils.h"
#include "../Objects/Core/Misc/FGuid.h"
#include "../Readers/FArchive.h"

namespace CUE4Parse::UE4::Versions
{
    namespace FOverlappingVerticesCustomVersion
    {
        using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

        enum Type
        {
            // Before any version changes were made in the plugin
            BeforeCustomVersionWasAdded = 0,
            // UE4.19
            // Converted to use HierarchicalInstancedStaticMeshComponent
            DetectOVerlappingVertices = 1,
            // -----<new versions can be added above this line>-------------------------------------------------
            VersionPlusOne,
            LatestVersion = VersionPlusOne - 1
        };

        inline const FGuid GUID(0x612FBE52, 0xDA53400B, 0x910D4F91, 0x9FB1857C);

        inline Type Get(Readers::FArchive& Ar)
        {
            const auto ver = CustomVer(Ar, GUID);
            if (ver >= 0) return static_cast<Type>(ver);

            const EGame game = Ar.Game();
            if (game < GAME_UE4_19) return BeforeCustomVersionWasAdded;
            return LatestVersion;
        }
    }
}
