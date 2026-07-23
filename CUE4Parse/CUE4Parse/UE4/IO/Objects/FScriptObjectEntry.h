// Ported from CUE4Parse/UE4/IO/Objects/FScriptObjectEntry.cs
// One global script-object record (from the ScriptObjects / LoaderInitialLoadMeta chunk): the object's
// name in the global name map plus its global/outer/CDO-class indices.
#pragma once

#include "FMappedName.h"
#include "FPackageObjectIndex.h"

namespace CUE4Parse::UE4::IO::Objects
{
    struct FScriptObjectEntry
    {
        FMappedName ObjectName;
        FPackageObjectIndex GlobalIndex;
        FPackageObjectIndex OuterIndex;
        FPackageObjectIndex CDOClassIndex;
    };
    static_assert(sizeof(FScriptObjectEntry) == 32);
}
