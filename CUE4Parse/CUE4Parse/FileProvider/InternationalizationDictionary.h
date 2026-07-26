// Ported from CUE4Parse/FileProvider/InternationalizationDictionary.cs
// The namespace -> (key -> localized string) table FText Base histories resolve against, plus the culture
// machinery that fills it: which cultures a build ships, how they are aliased, and the .locres sweep
// ChangeCulture runs over the mounted files.
//
// Deliberate differences from C#:
//   * C# implements IReadOnlyDictionary; here the same surface is spelled out as named members
//     (SafeGet / Override / TryGetValue / ContainsKey / Count / Clear / ForEach).
//   * C#'s ConcurrentDictionary + per-namespace locks become a plain std::map, and Parallel.ForEach in
//     LoadByPattern becomes a serial scan — the port has no threading layer. This also removes the reason
//     for C#'s _provider field, whose only other uses are the StreamedFileProvider parallelism throttle and
//     the Aion2 branch below.
//   * ChangeCulture's Aion2 arm (LoadAion2L10NDatFiles, reading L10NString.dat through FAion2L10NFile) is
//     deferred: GameTypes/Aion2/Objects/FAion2L10NFile is still a stub. TODO.
//   * C#'s commented-out LocalizationPaths branch is preserved as a comment, because LocalizationPaths is
//     still parsed and exposed.
//   * TryGetCulture returns bool + an out string, like C#; ChangeCulture throws std::out_of_range where C#
//     throws KeyNotFoundException.
#pragma once

#include <map>
#include <string>
#include <vector>

#include "../UE4Config/Parsing/ConfigIni.h"
#include "../Utils/StringComparer.h"

namespace CUE4Parse::UE4::Localization { class FTextLocalizationMetaDataResource; }

namespace CUE4Parse::FileProvider
{
    namespace Vfs { class FileProviderDictionary; }

    class InternationalizationDictionary
    {
    public:
        // C# hands the dictionary the provider's PathComparer; it decides whether a culture name matches
        // case-sensitively, both in AvailableCultures and in the CultureMappings table.
        explicit InternationalizationDictionary(Utils::StringComparer comparer = Utils::StringComparer::Ordinal())
            : _comparer(comparer), _cultureMappings(comparer) {}

        // The localized string for namespace/key, or defaultValue when it is not in the table.
        std::string SafeGet(const std::string& ns, const std::string& key, const std::string& defaultValue = "") const
        {
            const auto nsIt = _collection.find(ns);
            if (nsIt != _collection.end())
            {
                const auto it = nsIt->second.find(key);
                if (it != nsIt->second.end())
                    return it->second;
            }
            return defaultValue;
        }

        // Merges the given entries over the table (C#'s Override).
        void Override(const std::map<std::string, std::map<std::string, std::string>>& dictionary)
        {
            for (const auto& [ns, entries] : dictionary)
            {
                auto& d = _collection[ns];
                for (const auto& [key, value] : entries)
                    d[key] = value;
            }
        }

        bool ContainsKey(const std::string& ns) const { return _collection.count(ns) != 0; }

        bool TryGetValue(const std::string& ns, const std::map<std::string, std::string>*& value) const
        {
            const auto it = _collection.find(ns);
            if (it == _collection.end()) return false;
            value = &it->second;
            return true;
        }

        // C#'s GetEnumerator: every (namespace, entries) pair.
        template <typename Fn>
        void ForEach(Fn&& visit) const
        {
            for (const auto& [ns, entries] : _collection) visit(ns, entries);
        }

        size_t Count() const
        {
            size_t n = 0;
            for (const auto& [ns, entries] : _collection) n += entries.size();
            return n;
        }

        void Clear() { _collection.clear(); }

        // C#'s InitFromIni: the culture tables an ini declares. Reads CulturesToStage from
        // /Script/UnrealEd.ProjectPackagingSettings plus CultureMappings and LocalizationPaths from
        // [Internationalization]; only `Add` (+Key=) instructions count, as in C#.
        void InitFromIni(const UE4Config::Parsing::ConfigIni& ini);

        // C#'s InitFromMeta: appends a .locmeta's compiled cultures. A .locmeta older than
        // AddedCompiledCultures carries none (null upstream) and is ignored.
        void InitFromMeta(const UE4::Localization::FTextLocalizationMetaDataResource& meta);

        // C#'s TryGetCulture: `culture` itself if the build ships it, else whatever CultureMappings aliases
        // it to if *that* is shipped. A mapping that resolves wins over a direct hit, as upstream.
        bool TryGetCulture(const std::string& culture, std::string& validated) const;

        // C#'s ChangeCulture: validates the culture, drops the current table, and reloads it from every
        // .locres under a matching culture directory. Throws std::out_of_range on an unknown culture.
        void ChangeCulture(const std::string& culture, const Vfs::FileProviderDictionary& files);

        const std::string& Culture() const { return _culture; }

        const std::vector<std::string>& AvailableCultures() const { return _availableCultures; }
        const std::map<std::string, std::string, Utils::StringComparer>& CultureMappings() const { return _cultureMappings; }
        const std::vector<std::string>& LocalizationPaths() const { return _localizationPaths; }

    private:
        // C#'s LoadByPattern: reads every mounted file whose path matches, as a .locres, and merges its
        // entries in. A file that cannot be opened is skipped; one that fails to parse throws out of
        // ChangeCulture, as it does upstream (C# surfaces it as an AggregateException from Parallel.ForEach).
        void LoadByPattern(const std::string& pattern, const Vfs::FileProviderDictionary& files);

        Utils::StringComparer _comparer;
        std::map<std::string, std::map<std::string, std::string>> _collection;
        std::vector<std::string> _availableCultures;
        std::map<std::string, std::string, Utils::StringComparer> _cultureMappings;
        std::vector<std::string> _localizationPaths;
        std::string _culture;
    };
}
