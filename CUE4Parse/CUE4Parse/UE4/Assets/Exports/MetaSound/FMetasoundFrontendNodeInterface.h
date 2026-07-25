// Ported from CUE4Parse/UE4/Assets/Exports/MetaSound/FMetasoundFrontendNodeInterface.cs
// [StructFallback]. The pins of one node instance.
#pragma once

#include <vector>

#include "FMetasoundFrontendVertex.h"

namespace CUE4Parse::UE4::Assets::Exports::MetaSound
{
    class FMetasoundFrontendNodeInterface
    {
    public:
        std::vector<FMetasoundFrontendVertex> Inputs;
        std::vector<FMetasoundFrontendVertex> Outputs;
        std::vector<FMetasoundFrontendVertex> Environment;

        FMetasoundFrontendNodeInterface() = default;

        explicit FMetasoundFrontendNodeInterface(const FStructFallback& fallback)
        {
            Inputs = PropertyUtil::GetStructArray<FMetasoundFrontendVertex>(fallback, "Inputs");
            Outputs = PropertyUtil::GetStructArray<FMetasoundFrontendVertex>(fallback, "Outputs");
            Environment = PropertyUtil::GetStructArray<FMetasoundFrontendVertex>(fallback, "Environment");
        }
    };
}
