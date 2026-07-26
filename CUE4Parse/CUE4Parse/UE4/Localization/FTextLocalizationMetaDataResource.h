// Ported from CUE4Parse/UE4/Localization/FTextLocalizationMetaDataResource.cs
// A compiled .locmeta: which culture a localization target was authored in, which .locres holds it, and
// (from AddedCompiledCultures on) every culture the target was compiled for.
//
// Deliberate differences from C#:
//   * C#'s `string[]? CompiledCultures` becomes a vector plus bHasCompiledCultures — null and empty mean
//     different things upstream (InternationalizationDictionary.InitFromMeta returns early on null).
//   * The JsonConverter attribute is not ported (no JSON export layer), and the Serilog warning on a failed
//     magic check becomes a comment; the recovery is unchanged.
//   * The "too new" message interpolates ELocResVersion.Latest, not ELocMetaVersion.Latest — an upstream
//     copy-paste slip, kept verbatim.
#pragma once

#include <string>
#include <vector>

#include "../Objects/Core/Misc/FGuid.h"
#include "../Objects/Core/i18N/ELocMetaVersion.h"

namespace CUE4Parse::UE4::Readers { class FArchive; }

namespace CUE4Parse::UE4::Localization
{
    using Objects::Core::i18N::ELocMetaVersion;

    class FTextLocalizationMetaDataResource
    {
    public:
        std::string NativeCulture;
        std::string NativeLocRes;
        std::vector<std::string> CompiledCultures;
        // C#'s `CompiledCultures is null` test: false below ELocMetaVersion::AddedCompiledCultures.
        bool bHasCompiledCultures = false;
        bool bIsUGC = false;

        explicit FTextLocalizationMetaDataResource(Readers::FArchive& Ar);

    private:
        static const Objects::Core::Misc::FGuid LocMetaMagic;
    };
}
