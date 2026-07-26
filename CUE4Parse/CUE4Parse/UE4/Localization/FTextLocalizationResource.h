// Ported from CUE4Parse/UE4/Localization/FTextLocalizationResource.cs
// A compiled .locres: namespace -> key -> translated string, for one culture of one localization target.
//
// Deliberate differences from C#:
//   * Entries is Dictionary<FTextKey, Dictionary<FTextKey, FEntry>> upstream. FTextKey overrides neither
//     Equals nor GetHashCode, so those dictionaries key on *reference* identity: every namespace and key
//     read from the file gets its own object, nothing is ever deduplicated or overwritten, and enumeration
//     comes back in insertion order. Insertion-ordered vectors reproduce that exactly; a std::map keyed on
//     Str would not (it would merge repeated namespaces and reorder them). See FTextKey.h.
//   * The JsonConverter attribute is not ported (no JSON export layer).
//   * Serilog warnings become comments — the port has no logging layer. Both sites (a failed magic check,
//     and an out-of-range string index) keep the same recovery behaviour.
//   * Four game-specific arms depend on classes that are still stubs and are deferred with a TODO at each
//     site: the CodeVein2 and EmbersofTheUncrowned encrypted string tables, and the NevernessToEverness
//     string table. They fall through to the standard read. The HonorofKingsWorld arm has no such
//     dependency and IS ported.
#pragma once

#include <string>
#include <utility>
#include <vector>

#include "../Objects/Core/Misc/FGuid.h"
#include "../Objects/Core/i18N/ELocResVersion.h"
#include "../Objects/Core/i18N/FEntry.h"
#include "../Objects/Core/i18N/FTextKey.h"
#include "../Objects/Core/i18N/FTextLocalizationResourceString.h"

namespace CUE4Parse::UE4::Readers { class FArchive; }

namespace CUE4Parse::UE4::Localization
{
    using Objects::Core::i18N::ELocResVersion;
    using Objects::Core::i18N::FEntry;
    using Objects::Core::i18N::FTextKey;
    using Objects::Core::i18N::FTextLocalizationResourceString;

    class FTextLocalizationResource
    {
    public:
        // C#'s inner Dictionary<FTextKey, FEntry> — see the header note on why this is a vector.
        using FKeyTable = std::vector<std::pair<FTextKey, FEntry>>;

        std::vector<std::pair<FTextKey, FKeyTable>> Entries;

        explicit FTextLocalizationResource(Readers::FArchive& Ar);

    private:
        static const Objects::Core::Misc::FGuid LocResMagic;

        static std::vector<FTextLocalizationResourceString> ReadLocResStringArray(
            Readers::FArchive& Ar, ELocResVersion versionNumber);
    };
}
