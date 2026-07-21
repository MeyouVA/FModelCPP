// Ported from CUE4Parse/UE4/Assets/Exports/Engine/UDataTable.cs
// A UDataTable export: a map of row name -> row value (a bag of tagged properties, FStructFallback).
//
// Deliberate differences from C#:
//   * RowMap is an insertion-ordered vector<pair<FName, FStructFallback>> rather than a Dictionary: FName has
//     no hash/ordering, C#'s Dictionary iterates in ~insertion order, and the row-lookup utility scans it
//     linearly anyway.
//   * C# loads the RowStruct as a UStruct (ptr.TryLoad<UStruct>) to drive row parsing; UStruct loading is
//     deferred, so we recover only the struct's *name* and always take the by-name FStructFallback path. That
//     is equivalent for tagged assets (both branches read the same tagged property stream; the struct is only
//     consulted on the deferred unversioned path).
//   * Game-specific quirks (HonorofKingsWorld's extra header + custom map, LostSoulAside's trailing metadata)
//     are omitted; the port takes the default path. TODO.
//   * WriteJson and the TryGetDataTableRow utility are omitted for now.
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "../UObject.h"
#include "../../Objects/FStructFallback.h"
#include "../../../Objects/UObject/FName.h"

namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }

namespace CUE4Parse::UE4::Assets::Exports::Engine
{
    using CUE4Parse::UE4::Objects::UObject::FName;
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;

    class UDataTable : public UObject
    {
    public:
        std::vector<std::pair<FName, FStructFallback>> RowMap;
        std::optional<std::string> RowStructName; // Set by an inheritor or during deserialization.

        void Deserialize(Readers::FAssetArchive& Ar, int64_t validPos) override;
    };
}
