// Ported from CUE4Parse/UE4/Versions/FCurveExpressionCustomVersion.cs
// C#'s `static class` becomes a namespace: `Type` and `GUID` keep their qualified spelling
// (FXxx::Type::Member also resolves, the enum being unscoped) and Get() stays a free function.
#pragma once

#include "EGame.h"
#include "VersionUtils.h"
#include "../Objects/Core/Misc/FGuid.h"
#include "../Readers/FArchive.h"

namespace CUE4Parse::UE4::Versions
{
    namespace FCurveExpressionCustomVersion
    {
        using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

        enum Type
        {
            // Before any version changes were made in niagara
            BeforeCustomVersionWasAdded = 0,

            // Serialized expressions
            SerializedExpressions,
            ExpressionDataInSharedObject,

            VersionPlusOne,
            LatestVersion = VersionPlusOne - 1,
        };

        inline const FGuid GUID(0xA26D36AE, 0x26935388, 0xA8C5CB96, 0x2B95B4AF);

        inline Type Get(Readers::FArchive& Ar)
        {
            const auto ver = CustomVer(Ar, GUID);
            if (ver >= 0) return static_cast<Type>(ver);

            const EGame game = Ar.Game();
            if (game < GAME_UE5_4) return SerializedExpressions;
            return ExpressionDataInSharedObject;
        }
    }
}
