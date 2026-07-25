// Ported from CUE4Parse/UE4/Assets/Exports/MetaSound/FMetasoundFrontendEdge.cs
// [StructFallback]. A connection between two vertices.
#pragma once

#include "../PropertyUtil.h"
#include "../../../Objects/Core/Misc/FGuid.h"

namespace CUE4Parse::UE4::Assets::Exports::MetaSound
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

    class FMetasoundFrontendEdge
    {
    public:
        FGuid FromNodeID;
        FGuid FromVertexID;
        FGuid ToNodeID;
        FGuid ToVertexID;

        FMetasoundFrontendEdge() = default;

        explicit FMetasoundFrontendEdge(const FStructFallback& fallback)
        {
            FromNodeID = PropertyUtil::GetOrDefault<FGuid>(fallback, "FromNodeID");
            FromVertexID = PropertyUtil::GetOrDefault<FGuid>(fallback, "FromVertexID");
            ToNodeID = PropertyUtil::GetOrDefault<FGuid>(fallback, "ToNodeID");
            ToVertexID = PropertyUtil::GetOrDefault<FGuid>(fallback, "ToVertexID");
        }
    };
}
