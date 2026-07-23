// Ported from CUE4Parse/UE4/Versions/FLiveLinkCustomVersion.cs
// Custom serialization version for all packages containing LiveLink dependent asset types
// C#'s `static class` becomes a namespace: `Type` and `GUID` keep their qualified spelling
// (FXxx::Type::Member also resolves, the enum being unscoped) and Get() stays a free function.
#pragma once

#include "EGame.h"
#include "VersionUtils.h"
#include "../Objects/Core/Misc/FGuid.h"
#include "../Readers/FArchive.h"

namespace CUE4Parse::UE4::Versions
{
    namespace FLiveLinkCustomVersion
    {
        using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

        enum Type
        {
            // Before any version changes were made in the plugin
            BeforeCustomVersionWasAdded = 0,

            NewLiveLinkRoleSystem,

            // -----<new versions can be added above this line>-----
            VersionPlusOne,
            LatestVersion = VersionPlusOne - 1
        };

        inline const FGuid GUID(0xab965196, 0x45d808fc, 0xb7d7228d, 0x78ad569e);

        inline Type Get(Readers::FArchive& Ar)
        {
            const auto ver = CustomVer(Ar, GUID);
            if (ver >= 0) return static_cast<Type>(ver);

            const EGame game = Ar.Game();
            if (game < GAME_UE4_23) return BeforeCustomVersionWasAdded;
            return LatestVersion;
        }
    }
}
