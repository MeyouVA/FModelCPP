// Ported from CUE4Parse/UE4/FMod/Metadata/StringTable.cs
// The STDT chunk: converts event FModGuids to human-readable names via a packed radix tree.
#pragma once

#include <optional>

#include "../Objects/FRadixTreePacked.h"
#include "../Enums/EStringTableType.h"

namespace CUE4Parse::UE4::FMod::Metadata
{
    class StringTable
    {
    public:
        Enums::EStringTableType Type{};
        std::optional<Objects::FRadixTreePacked> RadixTree;

        explicit StringTable(Readers::FArchive& Ar)
        {
            Type = static_cast<Enums::EStringTableType>(Ar.Read<uint32_t>());

            if (Type == Enums::EStringTableType::StringTable_RadixTree_24Bit)
                RadixTree.emplace(Ar, Type);
        }
    };
}
