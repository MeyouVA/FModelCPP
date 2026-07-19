// Ported from CUE4Parse/UE4/Assets/Objects/Properties/MulticastDelegateProperty.cs (ctor).
#include "MulticastDelegateProperty.h"

#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    using CUE4Parse::UE4::Objects::UObject::FScriptDelegate;

    MulticastDelegateProperty::MulticastDelegateProperty(FAssetArchive& Ar, ReadType type)
        : Value(type == ReadType::ZERO ? FMulticastScriptDelegate(std::vector<FScriptDelegate>{})
                                       : FMulticastScriptDelegate(Ar))
    {
    }
}
