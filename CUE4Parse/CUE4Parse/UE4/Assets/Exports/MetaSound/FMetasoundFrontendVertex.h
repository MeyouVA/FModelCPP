// Ported from CUE4Parse/UE4/Assets/Exports/MetaSound/FMetasoundFrontendVertex.cs
// [StructFallback]. One pin on a node.
#pragma once

#include "../PropertyUtil.h"
#include "../../../Objects/Core/Misc/FGuid.h"
#include "../../../Objects/UObject/FName.h"

namespace CUE4Parse::UE4::Assets::Exports::MetaSound
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Objects::Core::Misc::FGuid;
    using CUE4Parse::UE4::Objects::UObject::FName;

    class FMetasoundFrontendVertex
    {
    public:
        FName Name;
        FName TypeName;
        FGuid VertexID;

        FMetasoundFrontendVertex() = default;
        virtual ~FMetasoundFrontendVertex() = default;

        explicit FMetasoundFrontendVertex(const FStructFallback& fallback)
        {
            Name = PropertyUtil::GetOrDefault<FName>(fallback, "Name");
            TypeName = PropertyUtil::GetOrDefault<FName>(fallback, "TypeName");
            VertexID = PropertyUtil::GetOrDefault<FGuid>(fallback, "VertexID");
        }
    };
}
