// Ported from CUE4Parse/UE4/Assets/Objects/Properties/SetProperty.cs (ctor).
#include "SetProperty.h"

#include "../FPropertyTagData.h"
#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    SetProperty::SetProperty(FAssetArchive& Ar, const FPropertyTagData* tagData, ReadType type)
    {
        if (type != ReadType::ZERO)
            Value = UScriptSet(Ar, tagData, type);
        // else: Value is an empty UScriptSet.
    }
}
