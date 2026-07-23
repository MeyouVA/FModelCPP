// Ported from CUE4Parse/UE4/Versions/VersionUtils.cs
// Resolves one custom-version key for an archive: the provider's override table wins, then the owning
// package's summary, and -1 means "no answer, guess from the game" (what every FXxxObjectVersion::Get does).
//
// Deliberate differences from C#:
//   * C#'s extension method `Ar.CustomVer(key)` becomes a free function `CustomVer(Ar, key)`; C++ has no
//     extension methods, and the call sites (the FXxx*Version::Get family) all live in this namespace.
//   * C# additionally tests `CustomVersionContainer: not null`. FPackageFileSummary holds the container by
//     value here, so it is always present; only the bUnversioned test remains.
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Readers { class FArchive; }
namespace CUE4Parse::UE4::Objects::Core::Misc { struct FGuid; }

namespace CUE4Parse::UE4::Versions
{
    int32_t CustomVer(Readers::FArchive& Ar, const Objects::Core::Misc::FGuid& key);
}
