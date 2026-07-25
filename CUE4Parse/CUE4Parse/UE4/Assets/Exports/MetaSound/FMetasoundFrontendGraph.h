// Ported from CUE4Parse/UE4/Assets/Exports/MetaSound/FMetasoundFrontendGraph.cs
// [StructFallback]. One page of a graph class: its nodes, edges and variables.
#pragma once

#include <vector>

#include "FMetasoundFrontendEdge.h"
#include "FMetasoundFrontendNode.h"
#include "FMetasoundFrontendVariable.h"
#include "../../../Objects/Core/Misc/FGuid.h"

namespace CUE4Parse::UE4::Assets::Exports::MetaSound
{
    class FMetasoundFrontendGraph
    {
    public:
        std::vector<FMetasoundFrontendNode> Nodes;
        std::vector<FMetasoundFrontendEdge> Edges;
        std::vector<FMetasoundFrontendVariable> Variables;
        FGuid PageID;

        FMetasoundFrontendGraph() = default;

        explicit FMetasoundFrontendGraph(const FStructFallback& fallback)
        {
            Nodes = PropertyUtil::GetStructArray<FMetasoundFrontendNode>(fallback, "Nodes");
            Edges = PropertyUtil::GetStructArray<FMetasoundFrontendEdge>(fallback, "Edges");
            Variables = PropertyUtil::GetStructArray<FMetasoundFrontendVariable>(fallback, "Variables");
            PageID = PropertyUtil::GetOrDefault<FGuid>(fallback, "PageID");
        }
    };
}
