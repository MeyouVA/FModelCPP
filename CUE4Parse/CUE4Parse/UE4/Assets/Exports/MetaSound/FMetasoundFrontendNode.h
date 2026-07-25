// Ported from CUE4Parse/UE4/Assets/Exports/MetaSound/FMetasoundFrontendNode.cs
// [StructFallback]. One instance of a class within a graph.
#pragma once

#include <vector>

#include "FMetasoundFrontendNodeInterface.h"
#include "FMetasoundFrontendVertexLiteral.h"
#include "../../../Objects/Core/Misc/FGuid.h"
#include "../../../Objects/UObject/FName.h"

namespace CUE4Parse::UE4::Assets::Exports::MetaSound
{
    using CUE4Parse::UE4::Objects::Core::Misc::FGuid;
    using CUE4Parse::UE4::Objects::UObject::FName;

    class FMetasoundFrontendNode
    {
    public:
        FGuid ID;
        FGuid ClassID;
        FName Name;
        FMetasoundFrontendNodeInterface Interface;
        std::vector<FMetasoundFrontendVertexLiteral> InputLiterals;

        FMetasoundFrontendNode() = default;

        explicit FMetasoundFrontendNode(const FStructFallback& fallback)
        {
            ID = PropertyUtil::GetOrDefault<FGuid>(fallback, "ID");
            ClassID = PropertyUtil::GetOrDefault<FGuid>(fallback, "ClassID");
            Name = PropertyUtil::GetOrDefault<FName>(fallback, "Name");
            Interface = PropertyUtil::GetOrDefault<FMetasoundFrontendNodeInterface>(fallback, "Interface");
            InputLiterals = PropertyUtil::GetStructArray<FMetasoundFrontendVertexLiteral>(fallback, "InputLiterals");
        }
    };
}
