// Ported from CUE4Parse/UE4/Assets/Objects/Properties/SoftObjectProperty.cs (ctor).
#include "SoftObjectProperty.h"

#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    SoftObjectProperty::SoftObjectProperty(FAssetArchive& Ar, ReadType type)
        : Value(type == ReadType::ZERO ? FSoftObjectPath() : FSoftObjectPath(Ar))
    {
    }
}
