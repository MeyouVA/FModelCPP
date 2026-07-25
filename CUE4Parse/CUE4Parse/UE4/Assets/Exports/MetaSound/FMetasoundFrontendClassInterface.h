// Ported from CUE4Parse/UE4/Assets/Exports/MetaSound/FMetasoundFrontendClassInterface.cs
// [StructFallback]. Everything a class exposes: its inputs, outputs and environment requirements.
#pragma once

#include <vector>

#include "FMetasoundFrontendClassEnvironmentVariable.h"
#include "FMetasoundFrontendClassInput.h"
#include "FMetasoundFrontendClassOutput.h"

namespace CUE4Parse::UE4::Assets::Exports::MetaSound
{
    class FMetasoundFrontendClassInterface
    {
    public:
        std::vector<FMetasoundFrontendClassInput> Inputs;
        std::vector<FMetasoundFrontendClassOutput> Outputs;
        std::vector<FMetasoundFrontendClassEnvironmentVariable> Environment;

        FMetasoundFrontendClassInterface() = default;

        explicit FMetasoundFrontendClassInterface(const FStructFallback& fallback)
        {
            Inputs = PropertyUtil::GetStructArray<FMetasoundFrontendClassInput>(fallback, "Inputs");
            Outputs = PropertyUtil::GetStructArray<FMetasoundFrontendClassOutput>(fallback, "Outputs");
            Environment = PropertyUtil::GetStructArray<FMetasoundFrontendClassEnvironmentVariable>(fallback, "Environment");
        }
    };
}
