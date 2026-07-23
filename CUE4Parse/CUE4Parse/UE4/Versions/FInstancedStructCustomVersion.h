// Ported from CUE4Parse/UE4/Versions/FInstancedStructCustomVersion.cs
// C#'s `static class` becomes a namespace: `Type` and `GUID` keep their qualified spelling
// (FXxx::Type::Member also resolves, the enum being unscoped) and Get() stays a free function.
#pragma once

#include "EGame.h"
#include "VersionUtils.h"
#include "../Objects/Core/Misc/FGuid.h"
#include "../Readers/FArchive.h"

namespace CUE4Parse::UE4::Versions
{
    namespace FInstancedStructCustomVersion
    {
        using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

        enum Type
        {
            // Before any version changes were made
            CustomVersionAdded = 0,

            // -----<new versions can be added above this line>-----
            VersionPlusOne,
            LatestVersion = VersionPlusOne - 1
        };

        inline const FGuid GUID(0xE21E1CAA, 0xAF47425E, 0x89BF6AD4, 0x4C44A8BB);

        inline Type Get(Readers::FArchive& Ar)
        {
            const auto ver = CustomVer(Ar, GUID);
            if (ver >= 0) return static_cast<Type>(ver);

            const EGame game = Ar.Game();
            if (game == GAME_ArenaBreakoutMobile) return CustomVersionAdded;
            if (game < GAME_UE5_3) return static_cast<Type>(-1);
            return LatestVersion;
        }
    }
}
