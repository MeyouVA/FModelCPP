// Ported from CUE4Parse/UE4/Localization/FTextLocalizationResource.cs
#include "FTextLocalizationResource.h"

#include <cstdint>

#include "../Exceptions/ParserException.h"
#include "../Readers/FArchive.h"
#include "../Versions/EGame.h"
#include "../../Utils/StringUtils.h"

namespace CUE4Parse::UE4::Localization
{
    using namespace CUE4Parse::UE4::Versions;
    using Objects::Core::Misc::FGuid;

    const FGuid FTextLocalizationResource::LocResMagic{0x7574140Eu, 0xFC034A67u, 0x9D90154Au, 0x1B7F37C3u};

    FTextLocalizationResource::FTextLocalizationResource(Readers::FArchive& Ar)
    {
        const auto locResMagic = Ar.Read<FGuid>();
        auto versionNumber = ELocResVersion::Legacy;
        if (locResMagic == LocResMagic)
        {
            versionNumber = Ar.Read<ELocResVersion>();
        }
        else // Legacy LocRes files lack the magic number, assume that's what we're dealing with, and seek back to the start of the file
        {
            Ar.Position = 0;
            // C#: Log.Warning($"LocRes '{Ar.Name}' failed the magic number check! Assuming this is a legacy
            // resource"). The port has no logging layer; the recovery (rewind and read as Legacy) is the same.
        }

        // Is this LocRes file too new to load?
        if (versionNumber > ELocResVersion::Latest)
        {
            if (!(Ar.Game() == GAME_StellarBlade || Ar.Game() == GAME_HonorofKingsWorld))
                throw Exceptions::ParserException(
                    Ar, "LocRes '" + Ar.Name() + "' is too new to be loaded (File Version: " +
                        std::to_string(static_cast<int>(versionNumber)) + ", Loader Version: " +
                        std::to_string(static_cast<int>(ELocResVersion::Latest)) + ")");
        }

        if (Ar.Game() == GAME_HonorofKingsWorld && versionNumber > ELocResVersion::Latest)
        {
            Ar.SkipFixedArray(sizeof(uint32_t));
            // C#'s tuple getter: (name, entryCount, unused). C# evaluates tuple elements left to right; the
            // reads are sequenced explicitly here because C++ would not guarantee that inside a braced list.
            const auto dts = Ar.ReadArrayWith([&]
            {
                std::string name = Ar.ReadFString();
                const int32_t entryCount = Ar.Read<int32_t>();
                const int32_t unused = Ar.Read<int32_t>(); // read and discarded upstream too
                (void) unused;
                return std::make_pair(std::move(name), entryCount);
            });

            FKeyTable dict;
            size_t total = 0;
            for (const auto& dt : dts) total += static_cast<size_t>(dt.second > 0 ? dt.second : 0);
            dict.reserve(total); // C#'s `new Dictionary<...>(dts.Sum(x => x.Item2))`

            for (const auto& dt : dts)
            {
                for (int32_t i = 0; i < dt.second; i++)
                {
                    const uint32_t keyHash = Ar.Read<uint32_t>();
                    std::string keyStr = Ar.ReadFString();
                    const uint32_t sourceStringHash = Ar.Read<uint32_t>();
                    std::string localizedString = Ar.ReadFString();
                    const std::string trailing = Ar.ReadFString(); // read and discarded upstream too
                    (void) trailing;
                    // C# assigns through the dictionary indexer, but FTextKey has reference equality, so
                    // this can only ever append (see the header note).
                    dict.emplace_back(FTextKey(std::move(keyStr), keyHash),
                                      FEntry(std::move(localizedString), Ar.Name(), sourceStringHash));
                }
            }
            Entries.emplace_back(FTextKey(""), std::move(dict));
            return;
        }

        // Read the localized string array
        std::vector<FTextLocalizationResourceString> localizedStringArray;
        if (versionNumber >= ELocResVersion::Compact)
        {
            localizedStringArray = ReadLocResStringArray(Ar, versionNumber);
        }

        // Read entries count
        if (versionNumber >= ELocResVersion::Optimized_CRC32)
        {
            Ar.Position += 4; // EntriesCount
        }

        // Read namespace count
        const auto namespaceCount = Ar.Read<uint32_t>();
        for (uint32_t i = 0; i < namespaceCount; i++)
        {
            FTextKey namespce(Ar, versionNumber);
            const auto keyCount = Ar.Read<uint32_t>();
            FKeyTable keyValue;
            keyValue.reserve(keyCount);
            for (uint32_t j = 0; j < keyCount; j++)
            {
                FTextKey key(Ar, versionNumber);
                FEntry newEntry(Ar);
                if (versionNumber >= ELocResVersion::Compact)
                {
                    const auto localizedStringIndex = Ar.Read<int32_t>();
                    if (localizedStringIndex >= 0 &&
                        static_cast<size_t>(localizedStringIndex) < localizedStringArray.size())
                    {
                        // Steal the string if possible
                        auto& localizedString = localizedStringArray[static_cast<size_t>(localizedStringIndex)];
                        newEntry.LocalizedString = localizedString.String;
                        if (localizedString.RefCount != -1) localizedString.RefCount--;
                    }
                    else
                    {
                        // C#: Log.Warning($"LocRes '{newEntry.LocResName}' has an invalid localized string
                        // index for namespace '{namespce.Str}' and key '{key.Str}'. This entry will have no
                        // translation."). The entry is still added, with an empty string.
                        //
                        // C#'s guard is `localizedStringArray.Length > localizedStringIndex`, which lets a
                        // negative index through into an IndexOutOfRangeException; the >= 0 test above turns
                        // that crash into the same "no translation" path. No shipped file hits it.
                    }

                    if (Ar.Game() == GAME_StellarBlade && versionNumber > ELocResVersion::Latest) Ar.Position += 4;
                }
                else
                {
                    newEntry.LocalizedString = Ar.ReadFString();
                }

                keyValue.emplace_back(std::move(key), std::move(newEntry));
            }
            Entries.emplace_back(std::move(namespce), std::move(keyValue));
        }
    }

    std::vector<FTextLocalizationResourceString> FTextLocalizationResource::ReadLocResStringArray(
        Readers::FArchive& Ar, ELocResVersion versionNumber)
    {
        // TODO: C# routes NevernessToEverness(_CBT2) archives under "HT/Content/Localization/" through
        // FNTEFTextLocalizationResource.ReadLocResStringArray, which decrypts the table. That class
        // (GameTypes/NTE/Encryption/NevernessToEvernessLocresEncryption) is still a stub, so those files fall
        // through to the standard read below and come back as garbage rather than throwing.

        const auto localizedStringArrayOffset = Ar.Read<int64_t>();
        if (localizedStringArrayOffset != -1) // INDEX_NONE
        {
            const auto currentFileOffset = Ar.Position;
            Ar.Position = localizedStringArrayOffset;
            // TODO: C# has two more arms here, both on stubbed classes: CodeVein2 archives under
            // "CodeVein2/Content/Localization/" read through CodeVein2StringEncryption (ECV2DecryptionMode
            // .Locres), and EmbersofTheUncrowned reads through EOTUStringEncryption.DecryptString. Both fall
            // through to the standard read.
            auto localizedStringArray = Ar.ReadArrayWith(
                [&] { return FTextLocalizationResourceString(Ar, versionNumber); });
            Ar.Position = currentFileOffset;
            return localizedStringArray;
        }

        return {};
    }
}
