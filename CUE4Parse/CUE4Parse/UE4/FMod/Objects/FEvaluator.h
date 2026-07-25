// Ported from CUE4Parse/UE4/FMod/Objects/FEvaluator.cs
// A tagged evaluator: the low byte of the raw uint is the type; the trailing data shape depends on it.
// C#'s `object? Data` becomes a std::variant of the possible payload shapes.
#pragma once

#include <array>
#include <stdexcept>
#include <variant>
#include <vector>

#include "FModGuid.h"
#include "../Enums/EEvaluatorType.h"

namespace CUE4Parse::UE4::FMod::Objects
{
    struct FEvaluator
    {
        Enums::EEvaluatorType Type{};
        // monostate = null (Basic*/Type12/Type30), uint32 (Type10), FModGuid (Type11), 2 floats (Type20).
        std::variant<std::monostate, uint32_t, FModGuid, std::array<float, 2>> Data;

        FEvaluator() = default;

        explicit FEvaluator(Readers::FArchive& Ar)
        {
            uint32_t rawType = Ar.Read<uint32_t>();
            Type = static_cast<Enums::EEvaluatorType>(rawType & 0xFF); // Only lower 8 bits are the type

            switch (Type)
            {
                case Enums::EEvaluatorType::Basic0:
                case Enums::EEvaluatorType::Basic1:
                case Enums::EEvaluatorType::Basic2:
                case Enums::EEvaluatorType::Basic3:
                case Enums::EEvaluatorType::Type12:
                case Enums::EEvaluatorType::Type30:
                    break;

                case Enums::EEvaluatorType::Type10:
                    Data = Ar.Read<uint32_t>();
                    break;

                case Enums::EEvaluatorType::Type11:
                    Data = FModGuid(Ar);
                    break;

                case Enums::EEvaluatorType::Type20:
                    Data = std::array<float, 2>{ Ar.Read<float>(), Ar.Read<float>() };
                    break;

                default:
                    throw std::runtime_error("Unknown evaluator type");
            }
        }

        static std::vector<FEvaluator> ReadEvaluatorList(Readers::FArchive& Ar)
        {
            std::vector<FEvaluator> evaluators;

            int32_t totalSize = Ar.Read<int32_t>();
            if (totalSize <= 0) return {};

            int64_t startPos = Ar.Position;
            int64_t endPos = startPos + totalSize;
            while (Ar.Position < endPos)
                evaluators.emplace_back(Ar);

            return evaluators;
        }
    };
}
