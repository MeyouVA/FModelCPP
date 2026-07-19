// Ported from CUE4Parse/UE4/Objects/UObject/FUniqueObjectGuid.cs
// A lazy-object reference stored as a single FGuid. Laid out as a trivially copyable value type so it can
// be read directly via FArchive::Read<FUniqueObjectGuid>() (matching C#'s Ar.Read<FUniqueObjectGuid>()).
#pragma once

#include "../Core/Misc/FGuid.h"

namespace CUE4Parse::UE4::Objects::UObject
{
    using Core::Misc::FGuid;

    struct FUniqueObjectGuid
    {
        FGuid Guid;

        std::string ToString() const { return Guid.ToString(); }
    };
}
