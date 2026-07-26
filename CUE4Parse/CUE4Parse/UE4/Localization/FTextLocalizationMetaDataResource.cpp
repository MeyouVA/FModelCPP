// Ported from CUE4Parse/UE4/Localization/FTextLocalizationMetaDataResource.cs
#include "FTextLocalizationMetaDataResource.h"

#include "../Exceptions/ParserException.h"
#include "../Objects/Core/i18N/ELocResVersion.h"
#include "../Readers/FArchive.h"

namespace CUE4Parse::UE4::Localization
{
    using Objects::Core::Misc::FGuid;
    using Objects::Core::i18N::ELocResVersion;

    const FGuid FTextLocalizationMetaDataResource::LocMetaMagic{0xA14CEE4Fu, 0x83554868u, 0xBD464C6Cu, 0x7C50DA70u};

    FTextLocalizationMetaDataResource::FTextLocalizationMetaDataResource(Readers::FArchive& Ar)
    {
        auto versionNumber = ELocMetaVersion::Initial;
        const auto locResMagic = Ar.Read<FGuid>();
        if (locResMagic == LocMetaMagic)
        {
            versionNumber = Ar.Read<ELocMetaVersion>();
        }
        else
        {
            Ar.Position = 0;
            // C#: Log.Warning("LocMeta '{name}' failed the magic number check!", Ar.Name). The port has no
            // logging layer; the recovery (rewind and read as Initial) is the same.
        }

        // Is this LocRes file too new to load?
        if (versionNumber > ELocMetaVersion::Latest)
        {
            // The loader version printed here is ELocResVersion::Latest upstream, not ELocMetaVersion's —
            // kept verbatim (see the header note).
            throw Exceptions::ParserException(
                Ar, "LocMeta '" + Ar.Name() + "' is too new to be loaded (File Version: " +
                    std::to_string(static_cast<int>(versionNumber)) + ", Loader Version: " +
                    std::to_string(static_cast<int>(ELocResVersion::Latest)) + ")");
        }

        NativeCulture = Ar.ReadFString();
        NativeLocRes = Ar.ReadFString();

        bHasCompiledCultures = versionNumber >= ELocMetaVersion::AddedCompiledCultures;
        if (bHasCompiledCultures)
            CompiledCultures = Ar.ReadArrayWith([&] { return Ar.ReadFString(); });

        bIsUGC = versionNumber >= ELocMetaVersion::AddedIsUGC && Ar.ReadBoolean();
    }
}
