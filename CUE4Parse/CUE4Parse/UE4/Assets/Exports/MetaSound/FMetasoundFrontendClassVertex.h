// Ported from CUE4Parse/UE4/Assets/Exports/MetaSound/FMetasoundFrontendClassVertex.cs
// A vertex as declared on a class (rather than on a node instance).
#pragma once

#include "FMetasoundFrontendVertex.h"

namespace CUE4Parse::UE4::Assets::Exports::MetaSound
{
    enum class EMetasoundFrontendVertexAccessType
    {
        Reference, //< The vertex accesses data by reference.
        Value,     //< The vertex accesses data by value.

        Unset      //< The vertex access level is unset (ex. vertex on an unconnected reroute node).
                   //< Not reflected as a graph core access type as core does not deal with reroutes
                   //< or ambiguous accessor level (it is resolved during document pre-processing).
    };

    class FMetasoundFrontendClassVertex : public FMetasoundFrontendVertex
    {
    public:
        FGuid NodeID;
        EMetasoundFrontendVertexAccessType AccessType = EMetasoundFrontendVertexAccessType::Reference;

        FMetasoundFrontendClassVertex() = default;

        explicit FMetasoundFrontendClassVertex(const FStructFallback& fallback) : FMetasoundFrontendVertex(fallback)
        {
            NodeID = PropertyUtil::GetOrDefault<FGuid>(fallback, "NodeID");
            AccessType = PropertyUtil::GetOrDefault<EMetasoundFrontendVertexAccessType>(fallback, "AccessType");
        }
    };
}
