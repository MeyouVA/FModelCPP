// Ported from CUE4Parse/UE4/Objects/UObject/FSoftObjectPath.cs (reading ctor, classic path only).
#include "FSoftObjectPath.h"

#include "../../Assets/Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Objects::UObject
{
    FSoftObjectPath::FSoftObjectPath(Assets::Readers::FAssetArchive& Ar)
    {
        // Classic path. See the header note for the deferred version/game branches.
        AssetPathName = Ar.ReadFName();
        SubPathString = Ar.ReadFString();
        Owner = Ar.Owner;
    }
}
