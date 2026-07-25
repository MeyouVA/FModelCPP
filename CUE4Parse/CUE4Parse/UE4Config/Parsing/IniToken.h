// Ported from the UE4Config.Parsing namespace of the Infrablack.UE4Config NuGet package, which the C#
// CUE4Parse takes as a dependency (AbstractFileProvider's DefaultGame/DefaultEngine are ConfigIni instances).
// There is no such package for C++, so the parsing subset CUE4Parse actually uses is vendored here under the
// same namespace and type names. The behaviour below was verified against the real 0.7.2.97 assembly.
//
// Deliberate differences from the C# package:
//   * Only the read path is ported: ConfigIniWriter, Write/CreateClone, the Hierarchy layer, and the
//     mutation helpers (MergeDuplicateSections, GroupPropertyInstructions, CondenseWhitespace,
//     NormalizeLineEndings, Sanitize, Cleanup) are omitted — CUE4Parse never calls them. TODO if the FModel
//     app ever needs to write an ini back out. Of the Evaluation layer, only PropertyEvaluator is vendored
//     (see Evaluation/PropertyEvaluator.h) — WwiseProvider and FModProvider read settings through it.
//   * C#'s TextLine is a struct with an implicit string conversion; here it is a plain aggregate.
#pragma once

#include <memory>
#include <string>
#include <vector>

namespace UE4Config::Parsing
{
    enum class LineEnding
    {
        Unknown,
        None,
        Unix,    // \n
        Windows, // \r\n
        Mac      // \r
    };

    enum class InstructionType
    {
        Set,      // Key=Value
        Add,      // +Key=Value
        AddForce, // .Key=Value
        Remove,   // -Key=Value
        RemoveAll // !Key
    };

    class IniToken
    {
    public:
        virtual ~IniToken() = default;
    };

    class LineToken : public IniToken
    {
    public:
        LineEnding Ending = LineEnding::Unknown;

        LineToken() = default;
        explicit LineToken(LineEnding ending) : Ending(ending) {}
    };

    struct TextLine
    {
        std::string Content;
        LineEnding Ending = LineEnding::Unknown;
    };

    // A token spanning one or more consecutive lines of the same kind (comments, runs of blank lines).
    class MultilineToken : public IniToken
    {
    public:
        std::vector<TextLine> Lines;

        void AddLine(const std::string& content, LineEnding ending) { Lines.push_back(TextLine{content, ending}); }
        std::vector<std::string> GetStringLines() const
        {
            std::vector<std::string> result;
            result.reserve(Lines.size());
            for (const auto& line : Lines) result.push_back(line.Content);
            return result;
        }
    };

    class CommentToken : public MultilineToken {};

    class WhitespaceToken : public MultilineToken {};

    // Any line that is neither a section header, a comment, whitespace nor a well-formed instruction.
    class TextToken : public LineToken
    {
    public:
        std::string Text;
    };

    class InstructionToken : public LineToken
    {
    public:
        InstructionType Type = InstructionType::Set;
        std::string Key;
        std::string Value;

        InstructionToken() = default;
        InstructionToken(InstructionType type, std::string key, std::string value, LineEnding ending = LineEnding::Unknown)
            : LineToken(ending), Type(type), Key(std::move(key)), Value(std::move(value)) {}
    };
}
