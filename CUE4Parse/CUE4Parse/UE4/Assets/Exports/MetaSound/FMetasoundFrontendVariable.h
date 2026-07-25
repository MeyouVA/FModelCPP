// Ported from CUE4Parse/UE4/Assets/Exports/MetaSound/FMetasoundFrontendVariable.cs
// [StructFallback]. A graph-scoped variable plus the node IDs that read and write it.
#pragma once

#include <vector>

#include "FMetasoundFrontendLiteral.h"
#include "../../../Objects/Core/Misc/FGuid.h"
#include "../../../Objects/UObject/FName.h"

namespace CUE4Parse::UE4::Assets::Exports::MetaSound
{
    using CUE4Parse::UE4::Objects::Core::Misc::FGuid;
    using CUE4Parse::UE4::Objects::UObject::FName;

    class FMetasoundFrontendVariable
    {
    public:
        FName Name;
        FName TypeName;
        FMetasoundFrontendLiteral Literal;
        FGuid ID;
        FGuid VariableNodeID;
        FGuid MutatorNodeID;
        std::vector<FGuid> AccessorNodeIDs;
        std::vector<FGuid> DeferredAccessorNodeIDs;

        FMetasoundFrontendVariable() = default;

        explicit FMetasoundFrontendVariable(const FStructFallback& fallback)
        {
            Name = PropertyUtil::GetOrDefault<FName>(fallback, "Name");
            TypeName = PropertyUtil::GetOrDefault<FName>(fallback, "TypeName");
            Literal = PropertyUtil::GetOrDefault<FMetasoundFrontendLiteral>(fallback, "Literal");
            ID = PropertyUtil::GetOrDefault<FGuid>(fallback, "ID");
            VariableNodeID = PropertyUtil::GetOrDefault<FGuid>(fallback, "VariableNodeID");
            MutatorNodeID = PropertyUtil::GetOrDefault<FGuid>(fallback, "MutatorNodeID");
            AccessorNodeIDs = PropertyUtil::GetArray<FGuid>(fallback, "AccessorNodeIDs");
            DeferredAccessorNodeIDs = PropertyUtil::GetArray<FGuid>(fallback, "DeferredAccessorNodeIDs");
        }
    };
}
