// Ported from CUE4Parse/UE4/Assets/Exports/Internationalization/UStringTable.cs.
#include "UStringTable.h"

#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Exports::Internationalization
{
    void UStringTable::Deserialize(Readers::FAssetArchive& Ar, int64_t validPos)
    {
        UObject::Deserialize(Ar, validPos);
        StringTable = FStringTable(Ar);
        // (DeltaForce .ustbin fallback omitted — see header.)
    }
}
