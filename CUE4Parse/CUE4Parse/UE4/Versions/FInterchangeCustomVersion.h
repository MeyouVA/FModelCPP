// Ported from CUE4Parse/UE4/Versions/FInterchangeCustomVersion.cs
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
    namespace FInterchangeCustomVersion
    {
        using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

        enum Type
        {
            // Roughly corresponds to 5.2
            BeforeCustomVersionWasAdded = 0,

            SerializedInterchangeObjectStoring,

            MultipleAllocationsPerAttributeInStorage,

            // The change that implemented the previous version had to be backed out to fix a serialization issue
            MultipleAllocationsPerAttributeInStorageFixed,

            // -----<new versions can be added above this line>-------------------------------------------------
            VersionPlusOne,
            LatestVersion = VersionPlusOne - 1
        };

        inline const FGuid GUID(0x92738C43, 0x29884D9C, 0x9A3D9BBE, 0x6EFF9FC0);

        inline Type Get(Readers::FArchive& Ar)
        {
            const auto ver = CustomVer(Ar, GUID);
            if (ver >= 0) return static_cast<Type>(ver);

            const EGame game = Ar.Game();
            if (game < GAME_UE5_2) return BeforeCustomVersionWasAdded;
            if (game < GAME_UE5_7) return SerializedInterchangeObjectStoring;
            return LatestVersion;
        }
    }
}
