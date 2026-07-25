// Ported from CUE4Parse/UE4/Assets/Exports/MetaSound/FMetasoundFrontendClassInput.cs
// [StructFallback].
#pragma once

#include <vector>

#include "FMetasoundFrontendClassInputDefault.h"
#include "FMetasoundFrontendClassVertex.h"

namespace CUE4Parse::UE4::Assets::Exports::MetaSound
{
    class FMetasoundFrontendClassInput : public FMetasoundFrontendClassVertex
    {
    public:
        std::vector<FMetasoundFrontendClassInputDefault> Defaults;

        FMetasoundFrontendClassInput() = default;

        explicit FMetasoundFrontendClassInput(const FStructFallback& fallback) : FMetasoundFrontendClassVertex(fallback)
        {
            Defaults = PropertyUtil::GetStructArray<FMetasoundFrontendClassInputDefault>(fallback, "Defaults");
        }
    };
}
