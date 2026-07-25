// Ported from the UE4Config.Evaluation namespace of the Infrablack.UE4Config NuGet package — see
// Parsing/IniToken.h for what is and is not vendored.
//
// This is the one piece of the Evaluation layer CUE4Parse actually uses: reducing a property's
// instructions ("Key=", "+Key=", ".Key=", "-Key=", "!Key") down to the list of values in effect.
// WwiseProvider and FModProvider both read their settings through it.
//
// Deliberate differences from the C# package:
//   * Only the ConfigIni overloads are vendored; the ConfigIniHierarchy / IEnumerable<ConfigIni> ones and
//     the token-collecting overloads are omitted, as is the settable Default evaluator (there is no
//     subclass to install). CustomOrDefault therefore reduces to "null means Default".
//   * C# takes IList<string>; here it is std::vector<std::string>.
//
// The instruction semantics below were checked against the real 0.7.2.97 assembly by calling
// PropertyEvaluator.Default.ExecutePropertyInstructions on constructed tokens. All comparisons are
// ordinal — an Add of "A" does not collide with an existing "a".
#pragma once

#include <string>
#include <vector>

#include "../Parsing/ConfigIni.h"
#include "../Parsing/IniToken.h"

namespace UE4Config::Evaluation
{
    using UE4Config::Parsing::ConfigIni;
    using UE4Config::Parsing::InstructionToken;

    class PropertyEvaluator
    {
    public:
        virtual ~PropertyEvaluator() = default;

        // C#'s static Default property. SetDefaultEvaluator/ResetDefaultEvaluator are not vendored, so this
        // is a plain singleton rather than a mutable static field.
        static PropertyEvaluator& Default();

        // C#'s static CustomOrDefault(evaluator).
        static PropertyEvaluator& CustomOrDefault(PropertyEvaluator* evaluator)
        { return evaluator != nullptr ? *evaluator : Default(); }

        // Applies one instruction to the running value list:
        //   Set       Key=Value   replaces the whole list with the single value
        //   Add       +Key=Value  appends, but only when the value is not already present
        //   AddForce  .Key=Value  appends unconditionally, duplicates and all
        //   Remove    -Key=Value  erases *every* occurrence of the value, not just the first
        //   RemoveAll !Key        clears the list; the token's value is ignored
        virtual void ExecutePropertyInstruction(const InstructionToken& instruction,
                                                std::vector<std::string>& values) const;

        void ExecutePropertyInstructions(const std::vector<const InstructionToken*>& instructions,
                                         std::vector<std::string>& values) const;

        // Collects sectionName/propertyName's instructions out of the whole file (every section with that
        // name, in file order) and folds them into `values`. Note that `values` is *not* cleared first: a
        // leading Set is what normally does that, exactly as in the package.
        void EvaluatePropertyValues(const ConfigIni& config, const std::string& sectionName,
                                    const std::string& propertyName, std::vector<std::string>& values) const;
    };
}
