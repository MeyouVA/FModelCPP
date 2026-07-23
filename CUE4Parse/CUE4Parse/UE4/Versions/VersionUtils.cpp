// Ported from CUE4Parse/UE4/Versions/VersionUtils.cs
#include "VersionUtils.h"

#include "VersionContainer.h"
#include "../Assets/IPackage.h"
#include "../Assets/Readers/FAssetArchive.h"
#include "../Objects/Core/Misc/FGuid.h"
#include "../Objects/Core/Serialization/FCustomVersionContainer.h"
#include "../Objects/UObject/FPackageFileSummary.h"
#include "../Readers/FArchive.h"

namespace CUE4Parse::UE4::Versions
{
    using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

    int32_t CustomVer(Readers::FArchive& Ar, const FGuid& key)
    {
        if (const auto& overrideCustomVersions = Ar.Versions.CustomVersions)
        {
            const int32_t overrideCustomVersion = overrideCustomVersions->GetVersion(key);
            if (overrideCustomVersion != -1)
                return overrideCustomVersion; // Return only if override
        }

        // C#'s `(Ar as FAssetArchive)?.Owner.Summary` — only an asset archive knows its package.
        if (const auto* assetAr = dynamic_cast<const Assets::Readers::FAssetArchive*>(&Ar))
        {
            const auto* summary = assetAr->Owner ? assetAr->Owner->GetSummary() : nullptr;
            if (summary && !summary->bUnversioned)
                return summary->CustomVersionContainer.GetVersion(key); // proceed so we can guess version from engine version
        }

        return -1; // Determine by game
    }
}
