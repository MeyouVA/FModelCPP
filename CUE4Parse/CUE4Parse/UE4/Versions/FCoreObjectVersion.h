// Ported from CUE4Parse/UE4/Versions/FCoreObjectVersion.cs
// Custom serialization version for changes made in Dev-Core stream
// C#'s `static class` becomes a namespace: `Type` and `GUID` keep their qualified spelling
// (FXxx::Type::Member also resolves, the enum being unscoped) and Get() stays a free function.
#pragma once

#include "EGame.h"
#include "VersionUtils.h"
#include "../Objects/Core/Misc/FGuid.h"
#include "../Readers/FArchive.h"

namespace CUE4Parse::UE4::Versions
{
    namespace FCoreObjectVersion
    {
        using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

        enum Type
        {
            // Before any version changes were made
            BeforeCustomVersionWasAdded = 0,
            MaterialInputNativeSerialize,
            EnumProperties,
            SkeletalMaterialEditorDataStripping,
            FProperties,

            // -----<new versions can be added above this line>-------------------------------------------------
            VersionPlusOne,
            LatestVersion = VersionPlusOne - 1
        };

        inline const FGuid GUID(0x375EC13C, 0x06E448FB, 0xB50084F0, 0x262A717E);

        inline Type Get(Readers::FArchive& Ar)
        {
            const auto ver = CustomVer(Ar, GUID);
            if (ver >= 0) return static_cast<Type>(ver);

            const EGame game = Ar.Game();
            if (game < GAME_UE4_12) return BeforeCustomVersionWasAdded;
            if (game < GAME_UE4_15) return MaterialInputNativeSerialize;
            if (game < GAME_UE4_22) return EnumProperties;
            if (game < GAME_UE4_25) return SkeletalMaterialEditorDataStripping;
            return FProperties;
        }
    }
}
