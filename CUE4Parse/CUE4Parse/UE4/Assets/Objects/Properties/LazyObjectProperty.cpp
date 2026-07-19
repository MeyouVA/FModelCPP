// Ported from CUE4Parse/UE4/Assets/Objects/Properties/LazyObjectProperty.cs (ctor).
#include "LazyObjectProperty.h"

#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    LazyObjectProperty::LazyObjectProperty(FAssetArchive& Ar, ReadType type)
    {
        Value = type == ReadType::ZERO ? FUniqueObjectGuid() : Ar.Read<FUniqueObjectGuid>();
    }
}
