// Ported from CUE4Parse/FileProvider/InternationalizationDictionary.cs (the non-inline members).
#include "InternationalizationDictionary.h"

#include <regex>
#include <stdexcept>

#include "Objects/GameFile.h"
#include "Vfs/FileProviderDictionary.h"
#include "../UE4/Localization/FTextLocalizationMetaDataResource.h"
#include "../UE4/Localization/FTextLocalizationResource.h"
#include "../UE4/Readers/FArchive.h"

namespace CUE4Parse::FileProvider
{
    using UE4Config::Parsing::InstructionToken;
    using UE4Config::Parsing::InstructionType;
    using UE4::Localization::FTextLocalizationMetaDataResource;
    using UE4::Localization::FTextLocalizationResource;

    void InternationalizationDictionary::InitFromIni(const UE4Config::Parsing::ConfigIni& ini)
    {
        _availableCultures.clear();
        _cultureMappings.clear();
        _localizationPaths.clear();

        std::vector<const InstructionToken*> instructions;
        ini.FindPropertyInstructions("/Script/UnrealEd.ProjectPackagingSettings", "CulturesToStage", instructions);
        for (const auto* instruction : instructions)
        {
            if (instruction->Type == InstructionType::Add)
                _availableCultures.push_back(instruction->Value);
        }

        instructions.clear();
        ini.FindPropertyInstructions("Internationalization", "CultureMappings", instructions);
        for (const auto* instruction : instructions)
        {
            if (instruction->Type != InstructionType::Add) continue;
            // C#: instruction.Value.Trim('"').Split(';') — "from;to".
            std::string value = instruction->Value;
            const size_t first = value.find_first_not_of('"');
            const size_t last = value.find_last_not_of('"');
            value = first == std::string::npos ? std::string() : value.substr(first, last - first + 1);
            const size_t separator = value.find(';');
            if (separator == std::string::npos) continue; // C# would throw on a malformed entry; skip it here
            _cultureMappings[value.substr(0, separator)] = value.substr(separator + 1);
        }

        instructions.clear();
        ini.FindPropertyInstructions("Internationalization", "LocalizationPaths", instructions);
        for (const auto* instruction : instructions)
        {
            if (instruction->Type == InstructionType::Add)
                _localizationPaths.push_back(instruction->Value);
        }
    }

    void InternationalizationDictionary::InitFromMeta(const FTextLocalizationMetaDataResource& meta)
    {
        if (!meta.bHasCompiledCultures) return; // C#'s `if (meta.CompiledCultures is null) return;`
        _availableCultures.insert(_availableCultures.end(), meta.CompiledCultures.begin(), meta.CompiledCultures.end());
    }

    bool InternationalizationDictionary::TryGetCulture(const std::string& culture, std::string& validated) const
    {
        bool found = false;
        validated.clear();

        for (const auto& available : _availableCultures)
        {
            if (_comparer.Equals(available, culture)) { validated = culture; found = true; break; }
        }

        // Note the missing `else`: upstream a resolvable mapping overwrites a direct hit.
        const auto mapping = _cultureMappings.find(culture);
        if (mapping != _cultureMappings.end())
        {
            for (const auto& available : _availableCultures)
            {
                if (_comparer.Equals(available, mapping->second)) { validated = mapping->second; found = true; break; }
            }
        }

        return found;
    }

    void InternationalizationDictionary::ChangeCulture(const std::string& culture, const Vfs::FileProviderDictionary& files)
    {
        std::string validated;
        if (!TryGetCulture(culture, validated))
            throw std::out_of_range("'" + culture + "' is not a valid culture.");

        _culture = validated;
        Clear();

        const std::string exclusion = "(?!Engine).+/";
        // Upstream keeps a LocalizationPaths-driven branch commented out here:
        //     foreach (var localizationPath in _localizationPaths)
        //         LoadByPattern($"^{localizationPath.Replace("%GAMEDIR%", exclusion)}/{Culture}/.+.locres$", files);
        // LocalizationPaths is still parsed and exposed, so it stays available if that branch is revived.
        LoadByPattern("^" + exclusion + ".+/" + _culture + "/.+.locres$", files);
        // TODO: C# also calls LoadAion2L10NDatFiles(Culture) for GAME_Aion2, reading every
        // ".../<culture>/L10NString.dat" through FAion2L10NFile. That class is still a stub.
    }

    void InternationalizationDictionary::LoadByPattern(const std::string& pattern, const Vfs::FileProviderDictionary& files)
    {
        // C#'s RegexOptions.IgnoreCase | Compiled. The '.' before "locres" is unescaped upstream, so it
        // matches any character; kept verbatim.
        const std::regex regex(pattern, std::regex::icase);

        files.ForEach([&](const std::string& path, const std::shared_ptr<Objects::GameFile>& file)
        {
            if (!std::regex_search(path, regex)) return;

            auto archive = file->SafeCreateReader();
            if (!archive) return;

            const FTextLocalizationResource locres(*archive);
            for (const auto& [namespce, entries] : locres.Entries)
            {
                auto& dictionary = _collection[namespce.Str];
                for (const auto& [key, entry] : entries)
                {
                    // TODO: we ignore the value priority here (upstream comment)
                    dictionary[key.Str] = entry.LocalizedString;
                }
            }
        });
    }
}
