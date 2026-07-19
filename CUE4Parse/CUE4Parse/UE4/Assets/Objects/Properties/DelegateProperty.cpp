// Ported from CUE4Parse/UE4/Assets/Objects/Properties/DelegateProperty.cs (ctor).
#include "DelegateProperty.h"

#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    using CUE4Parse::UE4::Objects::UObject::FPackageIndex;
    using CUE4Parse::UE4::Objects::UObject::FName;

    DelegateProperty::DelegateProperty(FAssetArchive& Ar, ReadType type)
        : Value(type == ReadType::ZERO ? FScriptDelegate(FPackageIndex(Ar, 0), FName()) : FScriptDelegate(Ar))
    {
    }
}
