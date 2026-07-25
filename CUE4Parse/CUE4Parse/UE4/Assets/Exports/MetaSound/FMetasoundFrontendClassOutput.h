// Ported from CUE4Parse/UE4/Assets/Exports/MetaSound/FMetasoundFrontendClassOutput.cs
// [StructFallback]. Adds nothing to the base, as in C#.
#pragma once

#include "FMetasoundFrontendClassVertex.h"

namespace CUE4Parse::UE4::Assets::Exports::MetaSound
{
    class FMetasoundFrontendClassOutput : public FMetasoundFrontendClassVertex
    {
    public:
        FMetasoundFrontendClassOutput() = default;
        explicit FMetasoundFrontendClassOutput(const FStructFallback& fallback) : FMetasoundFrontendClassVertex(fallback) {}
    };
}
