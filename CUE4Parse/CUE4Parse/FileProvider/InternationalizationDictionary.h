// Ported from CUE4Parse/FileProvider/InternationalizationDictionary.cs (lookup subset).
// The namespace -> (key -> localized string) table FText Base histories resolve against.
//
// Deliberate differences from C#:
//   * Only the lookup/population core is ported: SafeGet, Override, TryGetValue, ContainsKey, Clear, Count.
//     The culture machinery (AvailableCultures/CultureMappings/ChangeCulture, InitFromIni/InitFromMeta) and
//     the .locres loading path (LoadByPattern -> FTextLocalizationResource; needs that format + the provider
//     file dictionary) are deferred. TODO: port FTextLocalizationResource, then ChangeCulture.
//   * C#'s ConcurrentDictionary + per-namespace locks become a plain std::map (no parallel locres loads yet).
#pragma once

#include <map>
#include <string>
#include <vector>

#include "../UE4Config/Parsing/ConfigIni.h"

namespace CUE4Parse::FileProvider
{
    class InternationalizationDictionary
    {
    public:
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

        const std::vector<std::string>& AvailableCultures() const { return _availableCultures; }
        const std::map<std::string, std::string>& CultureMappings() const { return _cultureMappings; }
        const std::vector<std::string>& LocalizationPaths() const { return _localizationPaths; }

    private:
        std::map<std::string, std::map<std::string, std::string>> _collection;
        std::vector<std::string> _availableCultures;
        std::map<std::string, std::string> _cultureMappings;
        std::vector<std::string> _localizationPaths;
    };
}
