// Ported from CUE4Parse/FileProvider/InternationalizationDictionary.cs (InitFromIni; the rest is header-inline).
#include "InternationalizationDictionary.h"

namespace CUE4Parse::FileProvider
{
    using UE4Config::Parsing::InstructionToken;
    using UE4Config::Parsing::InstructionType;

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
}
