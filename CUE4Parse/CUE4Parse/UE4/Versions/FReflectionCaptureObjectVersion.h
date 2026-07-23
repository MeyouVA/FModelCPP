// Ported from CUE4Parse/UE4/Versions/FReflectionCaptureObjectVersion.cs
// Custom serialization version for changes made for a private stream
// C#'s `static class` becomes a namespace: `Type` and `GUID` keep their qualified spelling
// (FXxx::Type::Member also resolves, the enum being unscoped) and Get() stays a free function.
#pragma once

#include "EGame.h"
#include "VersionUtils.h"
#include "../Objects/Core/Misc/FGuid.h"
#include "../Readers/FArchive.h"

namespace CUE4Parse::UE4::Versions
{
    namespace FReflectionCaptureObjectVersion
    {
        using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

        enum Type
        {
            // Before any version changes were made
            BeforeCustomVersionWasAdded = 0,

            // Allows uncompressed reflection captures for cooked builds
            MoveReflectionCaptureDataToMapBuildData,

            // -----<new versions can be added above this line>-------------------------------------------------
            VersionPlusOne,
            LatestVersion = VersionPlusOne - 1
        };

        inline const FGuid GUID(0x6B266CEC, 0x1EC74B8F, 0xA30BE4D9, 0x0942FC07);

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
