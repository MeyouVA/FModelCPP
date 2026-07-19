// Ported from CUE4Parse/UE4/Objects/UObject/FFieldPath.cs (reading ctor, classic path).
#include "FFieldPath.h"

#include "../../Assets/Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Objects::UObject
{
    FFieldPath::FFieldPath(Assets::Readers::FAssetArchive& Ar)
    {
        Path = Ar.ReadArrayWith([&Ar] { return Ar.ReadFName(); });
        // The old serialization format could save 'None' paths; they should just be empty.
        if (Path.size() == 1 && Path[0].IsNone()) Path.clear();

        // Owner serialization: see the header note on the deferred custom-version gate.
        ResolvedOwner = FPackageIndex(Ar);
    }
}
