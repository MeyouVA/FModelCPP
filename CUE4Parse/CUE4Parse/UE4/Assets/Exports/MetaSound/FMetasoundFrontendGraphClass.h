// Ported from CUE4Parse/UE4/Assets/Exports/MetaSound/FMetasoundFrontendGraphClass.cs
// [StructFallback]. A class whose implementation is a graph rather than compiled code.
#pragma once

#include <vector>

#include "FMetasoundFrontendClass.h"
#include "FMetasoundFrontendGraph.h"

namespace CUE4Parse::UE4::Assets::Exports::MetaSound
{
    class FMetasoundFrontendGraphClass : public FMetasoundFrontendClass
    {
    public:
        std::vector<FMetasoundFrontendGraph> PagedGraphs;

        FMetasoundFrontendGraphClass() = default;

        explicit FMetasoundFrontendGraphClass(const FStructFallback& fallback) : FMetasoundFrontendClass(fallback)
        {
            PagedGraphs = PropertyUtil::GetStructArray<FMetasoundFrontendGraph>(fallback, "PagedGraphs");
        }
    };
}
