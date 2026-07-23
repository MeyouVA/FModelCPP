// Ported from CUE4Parse/UE4/Versions/FBlueprintsObjectVersion.cs
// Custom serialization version for changes made in Dev-Blueprints stream
// C#'s `static class` becomes a namespace: `Type` and `GUID` keep their qualified spelling
// (FXxx::Type::Member also resolves, the enum being unscoped) and Get() stays a free function.
#pragma once

#include "EGame.h"
#include "VersionUtils.h"
#include "../Objects/Core/Misc/FGuid.h"
#include "../Readers/FArchive.h"

namespace CUE4Parse::UE4::Versions
{
    namespace FBlueprintsObjectVersion
    {
        using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

        enum Type
        {
            // Before any version changes were made
            BeforeCustomVersionWasAdded = 0,
            OverridenEventReferenceFixup,
            CleanBlueprintFunctionFlags,
            ArrayGetByRefUpgrade,
            EdGraphPinOptimized,
            AllowDeletionConformed,
            AdvancedContainerSupport,
            SCSHasComponentTemplateClass,
            ComponentTemplateClassSupport,
            ArrayGetFuncsReplacedByCustomNode,
            DisallowObjectConfigVars,

            // -----<new versions can be added above this line>-------------------------------------------------
            VersionPlusOne,
            LatestVersion = VersionPlusOne - 1
        };

        inline const FGuid GUID(0xB0D832E4, 0x1F894F0D, 0xACCF7EB7, 0x36FD4AA2);

        inline Type Get(Readers::FArchive& Ar)
        {
            const auto ver = CustomVer(Ar, GUID);
            if (ver >= 0) return static_cast<Type>(ver);

            const EGame game = Ar.Game();
            return LatestVersion;
        }
    }
}
