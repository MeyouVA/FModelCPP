// Ported from CUE4Parse/UE4/IO/IoGlobalData.cs
// The data read once from the global IO Store container (global.utoc): the global name map and the
// script-object table Zen packages resolve their script imports against.
//
// Deliberate difference from C#: the FPackageObjectIndex-keyed Dictionary becomes an ordered std::map
// (FPackageObjectIndex has operator< on TypeAndId).
#pragma once

#include <map>
#include <vector>

#include "Objects/FPackageObjectIndex.h"
#include "Objects/FScriptObjectEntry.h"
#include "../Objects/UObject/FNameEntrySerialized.h"

namespace CUE4Parse::UE4::IO
{
    class IoStoreReader;

    class IoGlobalData
    {
    public:
        std::vector<CUE4Parse::UE4::Objects::UObject::FNameEntrySerialized> GlobalNameMap;
        std::map<Objects::FPackageObjectIndex, Objects::FScriptObjectEntry> ScriptObjectEntriesMap;

        explicit IoGlobalData(IoStoreReader& globalReader);
    };
}
