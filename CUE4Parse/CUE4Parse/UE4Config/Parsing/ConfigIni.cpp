// Ported from the UE4Config.Parsing namespace of the Infrablack.UE4Config NuGet package (read path only).
#include "ConfigIni.h"

namespace UE4Config::Parsing
{
    namespace
    {
        bool IsSpace(char c) { return c == ' ' || c == '\t' || c == '\v' || c == '\f'; }

        size_t FirstNonSpace(const std::string& s)
        {
            size_t i = 0;
            while (i < s.size() && IsSpace(s[i])) ++i;
            return i;
        }

        // Index one past the last non-whitespace character (0 for an all-whitespace string).
        size_t LastNonSpaceEnd(const std::string& s)
        {
            size_t end = s.size();
            while (end > 0 && IsSpace(s[end - 1])) --end;
            return end;
        }

        // Appends `line` to the trailing token when it is already a T, otherwise starts a new one.
        template <typename T>
        void AppendMultiline(ConfigIniSection& section, const std::string& line, LineEnding ending)
        {
            auto* last = dynamic_cast<T*>(section.GetLastToken());
            if (last == nullptr)
            {
                auto token = std::make_unique<T>();
                last = token.get();
                section.Tokens.push_back(std::move(token));
            }
            last->AddLine(line, ending);
        }
    }

    void ConfigIniSection::FindPropertyInstructions(const std::string& propertyName,
                                                    std::vector<const InstructionToken*>& outInstructions) const
    {
        for (const auto& token : Tokens)
        {
            const auto* instruction = dynamic_cast<const InstructionToken*>(token.get());
            if (instruction != nullptr && instruction->Key == propertyName)
                outInstructions.push_back(instruction);
        }
    }

    void ConfigIni::Read(const std::string& text)
    {
        Sections.clear();
        AppendRawText(text);
    }

    void ConfigIni::AppendRawText(const std::string& text)
    {
        // Every file opens with the unnamed section that holds anything before the first [Header].
        if (Sections.empty())
            Sections.push_back(std::make_unique<ConfigIniSection>());
        ConfigIniSection* currentSection = Sections.back().get();

        size_t pos = 0;
        while (pos <= text.size())
        {
            const size_t nl = text.find_first_of("\r\n", pos);
            if (nl == std::string::npos)
            {
                // Trailing content without a line ending; an empty tail after the last ending reads nothing.
                if (pos < text.size())
                    ReadLineWithoutLineEnding(text.substr(pos), LineEnding::None, currentSection);
                break;
            }

            LineEnding ending;
            size_t next;
            if (text[nl] == '\r' && nl + 1 < text.size() && text[nl + 1] == '\n')
            {
                ending = LineEnding::Windows;
                next = nl + 2;
            }
            else
            {
                ending = text[nl] == '\n' ? LineEnding::Unix : LineEnding::Mac;
                next = nl + 1;
            }

            ReadLineWithoutLineEnding(text.substr(pos, nl - pos), ending, currentSection);
            pos = next;
        }
    }

    void ConfigIni::ReadLineWithoutLineEnding(const std::string& line, LineEnding ending,
                                              ConfigIniSection*& currentSection)
    {
        const size_t start = FirstNonSpace(line);
        const size_t end = LastNonSpaceEnd(line);

        // Blank / whitespace-only line.
        if (start >= end)
        {
            AppendMultiline<WhitespaceToken>(*currentSection, line, ending);
            return;
        }

        // Section header: [Name], allowing surrounding whitespace (kept as the section's line waste).
        if (line[start] == '[' && line[end - 1] == ']')
        {
            auto section = std::make_unique<ConfigIniSection>(line.substr(start + 1, end - start - 2));
            section->LineWastePrefix = line.substr(0, start);
            section->LineWasteSuffix = line.substr(end);
            section->Ending = ending;
            currentSection = section.get();
            Sections.push_back(std::move(section));
            return;
        }

        // Comment: ';' as the first non-whitespace character ('#' is *not* a comment marker here).
        if (line[start] == ';')
        {
            AppendMultiline<CommentToken>(*currentSection, line, ending);
            return;
        }

        // Instructions are matched against the raw line, so a leading space demotes them to text.
        InstructionType type = InstructionType::Set;
        size_t keyStart = 0;
        switch (line[0])
        {
            case '+': type = InstructionType::Add;       keyStart = 1; break;
            case '.': type = InstructionType::AddForce;  keyStart = 1; break;
            case '-': type = InstructionType::Remove;    keyStart = 1; break;
            case '!': type = InstructionType::RemoveAll; keyStart = 1; break;
            default: break;
        }

        // RemoveAll takes the whole remainder as its key and has no value; every other kind needs a '='.
        if (type == InstructionType::RemoveAll)
        {
            currentSection->Tokens.push_back(
                std::make_unique<InstructionToken>(type, line.substr(keyStart), std::string(), ending));
            return;
        }

        const size_t equals = line.find('=', keyStart);
        if (equals == std::string::npos)
        {
            auto token = std::make_unique<TextToken>();
            token->Text = line;
            token->Ending = ending;
            currentSection->Tokens.push_back(std::move(token));
            return;
        }

        currentSection->Tokens.push_back(std::make_unique<InstructionToken>(
            type, line.substr(keyStart, equals - keyStart), line.substr(equals + 1), ending));
    }

    void ConfigIni::FindPropertyInstructions(const std::string& sectionName, const std::string& propertyName,
                                             std::vector<const InstructionToken*>& outInstructions) const
    {
        for (const auto& section : Sections)
        {
            if (section->Name == sectionName)
                section->FindPropertyInstructions(propertyName, outInstructions);
        }
    }

    const ConfigIniSection* ConfigIni::FindSection(const std::string& sectionName) const
    {
        for (const auto& section : Sections)
        {
            if (section->Name == sectionName) return section.get();
        }
        return nullptr;
    }
}
