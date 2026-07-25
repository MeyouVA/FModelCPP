// Ported from the UE4Config.Evaluation namespace of the Infrablack.UE4Config NuGet package (read path only).
#include "PropertyEvaluator.h"

#include <algorithm>

namespace UE4Config::Evaluation
{
    using UE4Config::Parsing::InstructionType;

    PropertyEvaluator& PropertyEvaluator::Default()
    {
        static PropertyEvaluator instance;
        return instance;
    }

    void PropertyEvaluator::ExecutePropertyInstruction(const InstructionToken& instruction,
                                                       std::vector<std::string>& values) const
    {
        switch (instruction.Type)
        {
            case InstructionType::Set:
                values.clear();
                values.push_back(instruction.Value);
                break;

            case InstructionType::Add:
                // "Add unique": the package skips the append when the value is already in the list.
                if (std::find(values.begin(), values.end(), instruction.Value) == values.end())
                    values.push_back(instruction.Value);
                break;

            case InstructionType::AddForce:
                values.push_back(instruction.Value);
                break;

            case InstructionType::Remove:
                values.erase(std::remove(values.begin(), values.end(), instruction.Value), values.end());
                break;

            case InstructionType::RemoveAll:
                // Clears everything -- the token carries no value to match against (see ConfigIni's parser,
                // which gives a RemoveAll line an empty Value).
                values.clear();
                break;
        }
    }

    void PropertyEvaluator::ExecutePropertyInstructions(const std::vector<const InstructionToken*>& instructions,
                                                        std::vector<std::string>& values) const
    {
        for (const InstructionToken* instruction : instructions)
        {
            if (instruction != nullptr)
                ExecutePropertyInstruction(*instruction, values);
        }
    }

    void PropertyEvaluator::EvaluatePropertyValues(const ConfigIni& config, const std::string& sectionName,
                                                   const std::string& propertyName,
                                                   std::vector<std::string>& values) const
    {
        std::vector<const InstructionToken*> instructions;
        config.FindPropertyInstructions(sectionName, propertyName, instructions);
        ExecutePropertyInstructions(instructions, values);
    }
}
