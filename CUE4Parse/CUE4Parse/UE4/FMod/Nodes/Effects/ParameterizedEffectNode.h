// Ported from CUE4Parse/UE4/FMod/Nodes/Effects/ParameterizedEffectNode.cs
#pragma once

#include <vector>

#include "../../Objects/FEffectParameter.h"
#include "../../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes::Effects
{
    class ParameterizedEffectNode
    {
    public:
        std::vector<Objects::FEffectParameter> Parameters;
        bool SideChainEnabled = false;

        explicit ParameterizedEffectNode(Readers::FArchive& Ar)
        {
            int32_t paramCount = Ar.Read<int32_t>();
            Parameters.reserve(paramCount > 0 ? static_cast<size_t>(paramCount) : 0);
            for (int i = 0; i < paramCount; i++)
                Parameters.emplace_back(Ar);

            if (FModReader::Version() >= 0x6e)
                SideChainEnabled = Ar.Read<uint8_t>() != 0;
        }
    };
}
