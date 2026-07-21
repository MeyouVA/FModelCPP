// Ported from CUE4Parse/UE4/Assets/Exports/UObjectRedirector.cs
#include "UObjectRedirector.h"

#include "../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Exports
{
    using Readers::FAssetArchive;

    void UObjectRedirector::Deserialize(FAssetArchive& Ar, int64_t validPos)
    {
        UObject::Deserialize(Ar, validPos);

        DestinationObject = FPackageIndex(Ar);
    }
}
