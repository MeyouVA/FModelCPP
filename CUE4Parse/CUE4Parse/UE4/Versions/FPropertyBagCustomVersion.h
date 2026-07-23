// Ported from CUE4Parse/UE4/Versions/FPropertyBagCustomVersion.cs
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
    namespace FPropertyBagCustomVersion
    {
        using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

        enum Type
        {
            // Before any version changes were made in the plugin
            BeforeCustomVersionWasAdded = 0,

            // Added support for array types
            ContainerTypes = 1,
            NestedContainerTypes = 2,
            MetaClass = 3,
            PropertyFlags = 4,
            KeyTypes = 5,

            // -----<new versions can be added above this line>-------------------------------------------------
            VersionPlusOne,
            LatestVersion = VersionPlusOne - 1,
        };

        inline const FGuid GUID(0x134A157E, 0xD5E249A3, 0x8D4E843C, 0x98FE9E31);

        inline Type Get(Readers::FArchive& Ar)
        {
            const auto ver = CustomVer(Ar, GUID);
            if (ver >= 0) return static_cast<Type>(ver);

            const EGame game = Ar.Game();
            if (game == GAME_BlackMythWukong) return NestedContainerTypes;
            if (game < GAME_UE5_1) return BeforeCustomVersionWasAdded;
            if (game < GAME_UE5_3) return ContainerTypes;
            if (game < GAME_UE5_4) return NestedContainerTypes;
            if (game < GAME_UE5_8) return MetaClass;
            return LatestVersion;
        }
    }
}
