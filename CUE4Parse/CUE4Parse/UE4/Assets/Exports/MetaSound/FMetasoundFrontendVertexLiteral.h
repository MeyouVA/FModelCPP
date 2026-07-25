// Ported from CUE4Parse/UE4/Assets/Exports/MetaSound/FMetasoundFrontendVertexLiteral.cs
// [StructFallback]. A literal bound to one vertex of a node.
#pragma once

#include "FMetasoundFrontendLiteral.h"
#include "../../../Objects/Core/Misc/FGuid.h"

namespace CUE4Parse::UE4::Assets::Exports::MetaSound
{
    using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

    class FMetasoundFrontendVertexLiteral
    {
    public:
        FGuid VertexID;
        FMetasoundFrontendLiteral Value;

        FMetasoundFrontendVertexLiteral() = default;

        explicit FMetasoundFrontendVertexLiteral(const FStructFallback& fallback)
        {
            VertexID = PropertyUtil::GetOrDefault<FGuid>(fallback, "VertexID");
            Value = PropertyUtil::GetOrDefault<FMetasoundFrontendLiteral>(fallback, "Value");
        }
    };
}
