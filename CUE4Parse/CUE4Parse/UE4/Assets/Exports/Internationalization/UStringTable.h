// Ported from CUE4Parse/UE4/Assets/Exports/Internationalization/UStringTable.cs.
// A UObject export holding an FStringTable. Registered with ObjectTypeRegistry under the serialized class
// name "StringTable", so a package export of that class deserializes into this type (and FText's
// StringTableEntry history can load it through the provider).
//
// Deliberate differences from C#:
//   * The DeltaForce empty-table fallback (loading a sibling .ustbin via FDeltaStringTable) is a per-game
//     override and omitted. TODO.
//   * WriteJson is omitted (no JSON layer yet).
#pragma once

#include "../UObject.h"
#include "FStringTable.h"

namespace CUE4Parse::UE4::Assets::Exports::Internationalization
{
    class UStringTable : public UObject
    {
    public:
        FStringTable StringTable;

        void Deserialize(Readers::FAssetArchive& Ar, int64_t validPos) override;
    };
}
