// Ported from CUE4Parse/UE4/Assets/Objects/Properties/ObjectProperty.cs (ctor).
#include "ObjectProperty.h"

#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    ObjectProperty::ObjectProperty(FAssetArchive& Ar, ReadType type)
        : Value(type == ReadType::ZERO ? FPackageIndex(Ar, 0) : FPackageIndex(Ar))
    {
    }
}
