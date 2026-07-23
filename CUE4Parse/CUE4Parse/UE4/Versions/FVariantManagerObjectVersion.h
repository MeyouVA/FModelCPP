// Ported from CUE4Parse/UE4/Versions/FVariantManagerObjectVersion.cs
// Custom serialization version for changes to variant manager objects
// C#'s `static class` becomes a namespace: `Type` and `GUID` keep their qualified spelling
// (FXxx::Type::Member also resolves, the enum being unscoped) and Get() stays a free function.
#pragma once

#include "EGame.h"
#include "VersionUtils.h"
#include "../Objects/Core/Misc/FGuid.h"
#include "../Readers/FArchive.h"

namespace CUE4Parse::UE4::Versions
{
    namespace FVariantManagerObjectVersion
    {
        using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

        enum Type
        {
            // Roughly corresponds to 4.21
            BeforeCustomVersionWasAdded = 0,

            CorrectSerializationOfFNameBytes,

            CategoryFlagsAndManualDisplayText,

            CorrectSerializationOfFStringBytes,

            SerializePropertiesAsNames,

            StoreDisplayOrder,

            // -----<new versions can be added above this line>-------------------------------------------------
            VersionPlusOne,
            LatestVersion = VersionPlusOne - 1
        };

        inline const FGuid GUID(0x24BB7AF3, 0x56464F83, 0x1F2F2DC2, 0x49AD96FF);

        inline Type Get(Readers::FArchive& Ar)
        {
            const auto ver = CustomVer(Ar, GUID);
            if (ver >= 0) return static_cast<Type>(ver);

            const EGame game = Ar.Game();
            if (game < GAME_UE4_22) return BeforeCustomVersionWasAdded;
            if (game < GAME_UE4_23) return SerializePropertiesAsNames;
            return LatestVersion;
        }
    }
}
