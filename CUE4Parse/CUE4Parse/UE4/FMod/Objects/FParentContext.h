// Ported from CUE4Parse/UE4/FMod/Objects/FParentContext.cs
// A stack frame used by FModReader while stitching a body chunk to its detail chunk.
#pragma once

#include "FModGuid.h"
#include "../Enums/ERIFFID.h"

namespace CUE4Parse::UE4::FMod::Objects
{
    struct FParentContext
    {
        Enums::ERIFFID NodeId{};
        FModGuid Guid;

        FParentContext() = default;
        FParentContext(Enums::ERIFFID nodeId, const FModGuid& guid) : NodeId(nodeId), Guid(guid) {}
    };
}
