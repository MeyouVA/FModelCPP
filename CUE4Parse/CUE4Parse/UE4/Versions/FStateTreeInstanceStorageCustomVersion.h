// Ported from CUE4Parse/UE4/Versions/FStateTreeInstanceStorageCustomVersion.cs
// C#'s `static class` becomes a namespace: `Type` and `GUID` keep their qualified spelling
// (FXxx::Type::Member also resolves, the enum being unscoped) and Get() stays a free function.
#pragma once

#include "EGame.h"
#include "VersionUtils.h"
#include "../Objects/Core/Misc/FGuid.h"
#include "../Readers/FArchive.h"

namespace CUE4Parse::UE4::Versions
{
    namespace FStateTreeInstanceStorageCustomVersion
    {
        using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

        enum Type
        {
            // Before any version changes were made in the plugin
            BeforeCustomVersionWasAdded = 0,
            // Added custom serialization
            AddedCustomSerialization,

            // -----<new versions can be added above this line>-----
            VersionPlusOne,
            LatestVersion = VersionPlusOne - 1
        };

        inline const FGuid GUID(0x60C4F0DE, 0x8B264C34, 0xAA937201, 0x5DFF09CC);

        inline Type Get(Readers::FArchive& Ar)
        {
            const auto ver = CustomVer(Ar, GUID);
            if (ver >= 0) return static_cast<Type>(ver);

            const EGame game = Ar.Game();
            if (game < GAME_UE5_4) return BeforeCustomVersionWasAdded;
            return LatestVersion;
        }
    }
}
