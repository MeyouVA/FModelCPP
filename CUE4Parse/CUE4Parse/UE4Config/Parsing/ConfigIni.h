// Ported from the UE4Config.Parsing namespace of the Infrablack.UE4Config NuGet package — see IniToken.h for
// what is and is not vendored. ConfigIni is the whole file, ConfigIniSection one [Header] block.
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "IniToken.h"

namespace UE4Config::Evaluation { class PropertyEvaluator; }

namespace UE4Config::Parsing
{
    class ConfigIniSection
    {
    public:
        std::string Name;
        // Whitespace that surrounded this section's [Header] line.
        std::string LineWastePrefix;
        std::string LineWasteSuffix;
        LineEnding Ending = LineEnding::Unknown;
        std::vector<std::unique_ptr<IniToken>> Tokens;

        ConfigIniSection() = default;
        explicit ConfigIniSection(std::string name) : Name(std::move(name)) {}

        // Appends every instruction in this section whose Key equals propertyName, in file order. Matching is
        // ordinal (the C# package uses string equality), and no instruction kind resets the list — a Set or a
        // RemoveAll is reported like any other, exactly as the package does.
        void FindPropertyInstructions(const std::string& propertyName,
                                      std::vector<const InstructionToken*>& outInstructions) const;

        // The last token, or null when the section is empty.
        IniToken* GetLastToken() const { return Tokens.empty() ? nullptr : Tokens.back().get(); }
    };

    class ConfigIni
    {
    public:
        std::string Name;
        std::vector<std::unique_ptr<ConfigIniSection>> Sections;

        ConfigIni() = default;
        explicit ConfigIni(std::string name) : Name(std::move(name)) {}
        virtual ~ConfigIni() = default;

        // C#'s Read(TextReader). This port takes the whole text instead: the C++ side has no TextReader layer,
        // and every caller already has the file's bytes in hand.
        void Read(const std::string& text);
        // C#'s AppendRawText: same parse, appending to whatever is already there.
        void AppendRawText(const std::string& text);

        // Collects the instructions for sectionName/propertyName across every section with that name.
        void FindPropertyInstructions(const std::string& sectionName, const std::string& propertyName,
                                      std::vector<const InstructionToken*>& outInstructions) const;

        // C#'s EvaluatePropertyValues(sectionName, propertyName, values, evaluator = null): folds the
        // property's instructions into `values` through the Evaluation layer. A null evaluator means the
        // default one. Declared here (as in the package, where ConfigIni references UE4Config.Evaluation)
        // but implemented in the .cpp, since the evaluator header includes this one.
        void EvaluatePropertyValues(const std::string& sectionName, const std::string& propertyName,
                                    std::vector<std::string>& values,
                                    Evaluation::PropertyEvaluator* evaluator = nullptr) const;

        // The first section with this name, or null.
        const ConfigIniSection* FindSection(const std::string& sectionName) const;

    private:
        // C#'s ReadLineWithoutLineEnding: classifies one line into the current section (or opens a new one).
        void ReadLineWithoutLineEnding(const std::string& line, LineEnding ending, ConfigIniSection*& currentSection);
    };
}
