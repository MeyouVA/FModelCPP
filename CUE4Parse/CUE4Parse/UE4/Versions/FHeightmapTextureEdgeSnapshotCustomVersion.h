// Ported from CUE4Parse/UE4/Versions/FHeightmapTextureEdgeSnapshotCustomVersion.cs
// Custom serialization version for changes to HeightmapTextureEdgeSnapshot
// C#'s `static class` becomes a namespace: `Type` and `GUID` keep their qualified spelling
// (FXxx::Type::Member also resolves, the enum being unscoped) and Get() stays a free function.
#pragma once

#include "EGame.h"
#include "VersionUtils.h"
#include "../Objects/Core/Misc/FGuid.h"
#include "../Readers/FArchive.h"

namespace CUE4Parse::UE4::Versions
{
    namespace FHeightmapTextureEdgeSnapshotCustomVersion
    {
        using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

        enum Type
        {
            BeforeCustomVersionWasAdded = 0,
            BeforeInitialHashWasAdded = 1,
            BeforeCornerDataWasRemoved = 2,
            BeforeChangedCornerHash = 3,
            BeforeChangedCookedFormat = 4,
            LatestVersion = 5
        };

        inline const FGuid GUID(0x12345678, 0x12345678, 0x12345678, 0x12345678);

        inline Type Get(Readers::FArchive& Ar)
        {
            const auto ver = CustomVer(Ar, GUID);
            if (ver >= 0) return static_cast<Type>(ver);

            const EGame game = Ar.Game();
            if (game < GAME_UE5_6) return BeforeCustomVersionWasAdded;
            return LatestVersion;
        }
    }
}
