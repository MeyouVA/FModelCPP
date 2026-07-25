// Ported from CUE4Parse/UE4/FMod/Nodes/Transitions/TransitionRegionNode.cs
#pragma once

#include <optional>
#include <vector>

#include "BaseTransitionNode.h"
#include "../../Objects/FModGuid.h"
#include "../../Objects/FLegacyParameterConditions.h"
#include "../../Objects/FEvaluator.h"
#include "../../Objects/FQuantization.h"
#include "../../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes::Transitions
{
    class TransitionRegionNode : public BaseTransitionNode
    {
    public:
        Objects::FModGuid BaseGuid;
        Objects::FModGuid DestinationGuid;
        uint32_t Start = 0;
        uint32_t End = 0;
        std::optional<Objects::FLegacyParameterConditions> LegacyParameterConditions;
        std::vector<Objects::FEvaluator> Evaluators;
        Objects::FQuantization Quantization;
        float TransitionChancePercent = 0.0f;
        uint32_t Flags = 0;

        explicit TransitionRegionNode(Readers::FArchive& Ar) : BaseGuid(Ar), DestinationGuid(Ar)
        {
            Start = Ar.Read<uint32_t>();
            End = Ar.Read<uint32_t>();

            if (FModReader::Version() < 0x43)
            {
                LegacyParameterConditions = Objects::FLegacyParameterConditions(Ar);
            }
            else if (FModReader::Version() >= 0x43 && FModReader::Version() < 0x82)
            {
                // C#'s ReadElemListImp<...>().FirstOrDefault(): the first element, or a default struct.
                auto list = FModReader::ReadElemListImp<Objects::FLegacyParameterConditions>(Ar);
                LegacyParameterConditions = list.empty() ? Objects::FLegacyParameterConditions{} : list.front();
            }
            else
            {
                Evaluators = Objects::FEvaluator::ReadEvaluatorList(Ar);
            }

            Quantization = Objects::FQuantization(Ar);
            TransitionChancePercent = Ar.Read<float>();

            if (FModReader::Version() >= 0x7f)
            {
                Flags = Ar.Read<uint32_t>();
            }
            else
            {
                Flags = Ar.Read<uint32_t>();

                // Extra legacy flags
                if (FModReader::Version() >= 0x38) (void) Ar.Read<uint8_t>();
                if (FModReader::Version() >= 0x67) (void) Ar.Read<uint8_t>();
                if (FModReader::Version() >= 0x79) (void) Ar.Read<uint8_t>();
                if (FModReader::Version() >= 0x7a) (void) Ar.Read<uint8_t>();
                if (FModReader::Version() >= 0x7b) (void) Ar.Read<uint8_t>();
            }
        }
    };
}
